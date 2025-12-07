#include <cstdio>
#include <iostream>
#include <string>
#include <unistd.h>
#include <vector>
#include <csignal>
#include <chrono>
#include <iomanip>

#include "ADS1115.h"
#include "unix.h" //THis example relies on the unix implementaion.

using std::cerr;
using std::cout;
using std::endl;
using std::exception;
using std::string;
using std::vector;

static volatile bool g_running = true;

void signal_handler(int signal)
{
  (void)signal;
  g_running = false;
}

void print_usage()
{
  cerr << "Usage: ADS1115_example ";
  cerr << "<i2c port address> " << endl;
  cerr << "<i2c address in hex> " << endl;
  cerr << "[channel] (optional: AIN0, AIN1, AIN2, AIN3, AIN0_AIN1, AIN0_AIN3, AIN1_AIN3, AIN2_AIN3, default: AIN0_AIN1)" << endl;
}

namespace
{
// Read from conversion register directly (for continuous mode)
template <typename T>
ADS1115::Error read_continuous(ADS1115::ADC<T> &adc, double& val)
{
  uint16_t reg = 0;
  auto err = adc.read_register(ADS1115::RegisterAddress::Conversion, reg);
  if (err == ADS1115::Error::NONE)
  {
    int16_t val_raw = static_cast<int16_t>(reg);
    // Convert raw value to voltage
    const auto fsr_v = ADS1115::get_fsr_voltage(adc.get_fsr());
    val = val_raw * (fsr_v / static_cast<double>(0x7FFF));
  }
  return err;
}

template <typename T>
void read_mux(ADS1115::ADC<T> &adc, ADS1115::Multiplex mux)
{
  double val;
  auto   err = adc.read(mux, val);
  cout << mux << " = " << val << " V | err = " << static_cast<int>(err) << endl;
}
}; // namespace

int run(int argc, char **argv)
{

  if (argc < 3)
  {
    print_usage();
    return 0;
  }

  // Argument 1 is the I2C port
  // Argument 2 is the address of the chip in hex
  // Argument 3 (optional) is the channel to read
  const auto port    = argv[1];
  uint8_t    address = std::stoul(argv[2], nullptr, 16);

  if (!ADS1115::is_valid_address(address))
  {
    cout << "Address invalid, possible addresses include: " << endl;
    cout << "\t ADR pin to GND: 0x" << std::hex << ADS1115::AddressPin::GND << endl;
    cout << "\t ADR pin to VDD: 0x" << std::hex << ADS1115::AddressPin::VDD << endl;
    cout << "\t ADR pin to SDA: 0x" << std::hex << ADS1115::AddressPin::SDA << endl;
    cout << "\t ADR pin to SCL: 0x" << std::hex << ADS1115::AddressPin::SCL << endl;
    return -1;
  }

  // Determine which channel to read (default: AIN0_AIN1 differential)
  ADS1115::Multiplex channel = ADS1115::Multiplex::AIN0_AIN1;
  if (argc >= 4)
  {
    string channel_str(argv[3]);
    if (channel_str == "AIN0") channel = ADS1115::Multiplex::AIN0;
    else if (channel_str == "AIN1") channel = ADS1115::Multiplex::AIN1;
    else if (channel_str == "AIN2") channel = ADS1115::Multiplex::AIN2;
    else if (channel_str == "AIN3") channel = ADS1115::Multiplex::AIN3;
    else if (channel_str == "AIN0_AIN1") channel = ADS1115::Multiplex::AIN0_AIN1;
    else if (channel_str == "AIN0_AIN3") channel = ADS1115::Multiplex::AIN0_AIN3;
    else if (channel_str == "AIN1_AIN3") channel = ADS1115::Multiplex::AIN1_AIN3;
    else if (channel_str == "AIN2_AIN3") channel = ADS1115::Multiplex::AIN2_AIN3;
    else
    {
      cerr << "Invalid channel: " << channel_str << endl;
      cerr << "Valid channels: AIN0, AIN1, AIN2, AIN3, AIN0_AIN1, AIN0_AIN3, AIN1_AIN3, AIN2_AIN3" << endl;
      return -1;
    }
  }

  cout << "Opening ADS1115 at " << port << " with address: 0x" << std::hex << static_cast<int>(address) << std::dec << endl;

  ADS1115::ADC<unix_i2c::i2c> adc(port, address);

  auto config_fsr = ADS1115::FullScaleRange::FSR_0_256V;
  auto config_dr  = ADS1115::DataRate::SPS_860;

  cout << "Setting FSR to ±" << config_fsr << endl;
  cout << "Setting DR to " << config_dr << " SPS" << endl;
  cout << "Setting channel to " << channel << endl;

  adc.set_fsr(config_fsr);
  adc.set_data_rate(config_dr);
  adc.set_multiplexing(channel);
  
  // Set to continuous conversion mode
  adc.set_conversion_mode(ADS1115::ConversionMode::Continuous);
  
  // Write config to start continuous conversion
  auto err = adc.write_config();
  if (err != ADS1115::Error::NONE)
  {
    cerr << "Failed to write configuration: " << static_cast<int>(err) << endl;
    return -1;
  }

  cout << "\nADC Configuration:" << endl;
  cout << "\tfsr             : ±" << adc.get_fsr() << endl;
  cout << "\tmultiplexing    : " << adc.get_multiplexing() << endl;
  cout << "\tdata rate       : " << adc.get_data_rate() << " SPS" << endl;
  cout << "\tconversion mode : " << adc.get_conversion_mode() << endl;
  cout << "\nStarting continuous reading (Press Ctrl+C to stop)...\n" << endl;

  // Wait a bit for first conversion to complete
  // At 860 SPS, one conversion takes ~1.16ms, so wait 5ms to be safe
  usleep(5000);

  auto start_time = std::chrono::steady_clock::now();
  int sample_count = 0;

  while (g_running)
  {
    double val;
    err = read_continuous<>(adc, val);
    
    if (err == ADS1115::Error::NONE)
    {
      auto current_time = std::chrono::steady_clock::now();
      auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time).count();
      
      cout << std::fixed << std::setprecision(6);
      cout << "[" << std::setw(8) << elapsed << " ms] ";
      cout << channel << " = " << std::setw(10) << val << " V";
      cout << " (sample #" << ++sample_count << ")" << endl;
    }
    else
    {
      cerr << "Read error: " << static_cast<int>(err) << endl;
    }

    // Small delay to match the conversion rate
    // At 860 SPS, one conversion takes ~1.16ms, so we read at ~1ms intervals
    // This allows reading at nearly the full data rate
    usleep(1000); // 1ms delay between reads (~1000 Hz reading rate)
  }

  cout << "\nStopped after " << sample_count << " samples." << endl;
  return 0;
}

int main(int argc, char **argv)
{
  // Set up signal handler for graceful shutdown
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  try
  {
    return run(argc, argv);
  }
  catch (exception &e)
  {
    cerr << "Unhandled Exception: " << e.what() << endl;
    return -1;
  }
}
