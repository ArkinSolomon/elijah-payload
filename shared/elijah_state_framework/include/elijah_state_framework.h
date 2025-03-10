#pragma once

#include <cstring>
#include <deque>
#include <functional>
#include <ranges>
#include <map>
#include <string>
#include <format>
#include <variant>
#include <pico/critical_section.h>
#include <pico/rand.h>
#include <pico/stdio_usb.h>

#include "data_type.h"
#include "fault_manager.h"
#include "persistent_data_storage.h"
#include "registered_command.h"
#include "state_framework_logger.h"
#include "variable_definition.h"

#define STDIO_WRITE_SIZE 128
constexpr uint64_t FRAMEWORK_TAG = 0xBC7AA65201C73901;

#define START_STATE_ENCODER(STATE_DATA_TYPE) void encode_state(void* encode_dest, const STATE_DATA_TYPE& state, const uint64_t seq, bool register_data) override \
  { \
  size_t data_len = 0; \
  size_t curr_data_size_len; \
  if (register_data) { \
    assert(!is_size_calculated); \
  } \
  curr_data_size_len = data_type_helpers::get_size_for_data_type(DataType::UInt64); \
  if (register_data) { \
    register_data_variable("_sequence", "", data_len, DataType::UInt64); \
  } else { \
    memcpy(static_cast<uint8_t*>(encode_dest) + data_len, &seq, curr_data_size_len); \
  } \
  data_len += curr_data_size_len; \
  if (register_data) { \
    register_data_variable("_us_since_boot", "", data_len, DataType::UInt64); \
  } else { \
    uint64_t us = to_us_since_boot(get_absolute_time()); \
    memcpy(static_cast<uint8_t*>(encode_dest) + data_len, &us, curr_data_size_len); \
  } \
  data_len += curr_data_size_len;
#define ENCODE_STATE(MEMBER_NAME, DATA_TYPE, DISP_NAME, DISP_UNIT) \
  assert(!done_with_static); \
  static_assert(!std::is_same<decltype(state.MEMBER_NAME), std::string>::value, "Property (" #MEMBER_NAME ") must be not be of type string"); \
  curr_data_size_len = data_type_helpers::get_size_for_data_type(DATA_TYPE); \
  if (register_data) { \
    register_data_variable(DISP_NAME, DISP_UNIT, data_len, DATA_TYPE); \
  } else { \
    memcpy(static_cast<uint8_t*>(encode_dest) + data_len, &state.MEMBER_NAME, curr_data_size_len); \
  } \
  data_len += curr_data_size_len;
#define END_STATE_ENCODER() \
    if (register_data) { \
      set_encoded_state_size(data_len); \
    } \
  }

enum class LogLevel : uint8_t
{
  Debug = 1,
  Default = 2,
  Warning = 3,
  Error = 4,
  SerialOnly = 5
};

template <typename StateDataType, EnumType PersistentKeyEnumType, EnumType FaultKeyEnumType>
class ElijahStateFramework
{
public:
  ElijahStateFramework(std::string application_name, PersistentKeyEnumType launch_key, size_t state_history_size);
  virtual ~ElijahStateFramework();

  PersistentDataStorage<PersistentKeyEnumType>* get_persistent_data_storage() const;

  [[nodiscard]] const std::string& get_application_name() const;

  [[nodiscard]] StateFrameworkLogger* get_logger() const;
  void lock_logger();
  void release_logger();

  static void log_serial_only(const std::string& message);
  void log_message(const std::string& message, LogLevel log_level = LogLevel::Default);
  void check_for_commands();

  void state_changed(const StateDataType& new_state);
  void lock_state_history();
  void release_state_history();
  [[nodiscard]] const std::deque<StateDataType>& get_state_history() const;

  void set_fault(FaultKeyEnumType fault_key, bool fault_state);
  void set_fault(FaultKeyEnumType fault_key, bool fault_state, const std::string& message);
  [[nodiscard]] bool is_faulted(FaultKeyEnumType fault_key);

protected:
  void register_command(const std::string& command, std::function<void(double)>&& callback);
  void register_command(const std::string& command, std::function<void()>&& callback);
  void register_command(const std::string& command, bool is_alphanumeric,
                        std::function<void(std::string)>&& callback);
  void register_command(const std::string& command, std::function<void(tm)>&& callback);

  void register_data_variable(const std::string& display_name, const std::string& display_unit, size_t offset,
                              DataType data_type);

  void register_fault(FaultKeyEnumType key, std::string fault_name, CommunicationChannel communication_channel);

  void set_encoded_state_size(size_t encoded_data_size);
  void finish_construction();

  void encode_state(void* encode_dest, const StateDataType& state);
  virtual void encode_state(void* encode_dest, const StateDataType& encode_data, uint64_t seq, bool register_data) = 0;

private:
  template <class... Ts>
  struct overloaded : Ts...
  {
    using Ts::operator()...;
  };

  template <class... Ts>
  overloaded(Ts...) -> overloaded<Ts...>;

  enum class OutputPacket : uint8_t
  {
    LogMessage = 1,
    StateUpdate = 2,
    PersistentStateUpdate = 3,
    Metadata = 4,
    DeviceRestartMarker = 5,
    FaultsChanged = 6
  };

  enum class MetadataSegment : uint8_t
  {
    ApplicationName = 1,
    Commands = 2,
    VariableDefinitions = 3,
    PersistentStorageEntries = 4,
    FaultInformation = 5,
    MetadataEnd = 255
  };

  static inline critical_section_t usb_cs;

  std::string application_name;
  PersistentKeyEnumType launch_key;

  uint8_t command_id_counter = 1;
  uint8_t variable_id_counter = 1;
  uint64_t state_seq = 1;

  std::map<uint8_t, RegisteredCommand> registered_commands;
  std::map<uint8_t, VariableDefinition> variable_definitions;

  bool is_size_calculated = false;
  size_t encoded_state_size = 0;

  shared_mutex_t state_history_smtx;
  size_t state_history_size;
  std::deque<StateDataType> state_history;

  PersistentDataStorage<PersistentKeyEnumType>* persistent_data_storage = new PersistentDataStorage<
    PersistentKeyEnumType>();
  StateFrameworkLogger* logger = nullptr;
  shared_mutex_t logger_smtx;

  FaultManager<FaultKeyEnumType>* fault_manager;

  static void initialize_communication();
  static void write_to_serial(const uint8_t* write_data, size_t write_len);
  static void write_to_serial(const uint8_t* packet_data, size_t packet_len, bool flush);

  static std::unique_ptr<uint8_t[]> encode_log_message(const std::string& message, LogLevel log_level,
                                                       size_t& encoded_len);

  void register_command(const std::string& command, CommandInputType command_input, command_callback_t callback);

  void send_framework_metadata(bool write_to_file);
  void send_persistent_state(const void* data, size_t data_len);
};

template <typename StateDataType, EnumType PersistentKeyEnumType, EnumType FaultKeyEnumType>
ElijahStateFramework<StateDataType, PersistentKeyEnumType, FaultKeyEnumType>::ElijahStateFramework(
  std::string application_name, PersistentKeyEnumType launch_key, size_t state_history_size) :
  application_name(std::move(application_name)), launch_key(launch_key), state_history_size(state_history_size)
{
  initialize_communication();
  shared_mutex_init(&logger_smtx);
  shared_mutex_init(&state_history_smtx);

  fault_manager = new FaultManager<FaultKeyEnumType>(0x0000);

  persistent_data_storage->on_commit([this](const void* data, const size_t data_len)
  {
    send_persistent_state(data, data_len);
  });

  register_command("_", [this]
  {
    send_framework_metadata(false);
  });

  // ReSharper disable once CppPassValueParameterByConstReference
  register_command("New launch", true, [this](std::string launch_name)
  {
    shared_mutex_enter_blocking_exclusive(&logger_smtx);
    logger->flush_log();
    delete logger;
    logger = new StateFrameworkLogger(launch_name); // NOLINT(*-unnecessary-value-param)
    shared_mutex_exit_exclusive(&logger_smtx);
  });

  register_command("Reset persistent storage", [this]
  {
    persistent_data_storage->load_default_data();
  });

  std::string rand_launch_name = std::format("launch-{:08x}", get_rand_64());
  persistent_data_storage->register_key(launch_key, "_launch_key", rand_launch_name);
}

template <typename StateDataType, EnumType PersistentKeyEnumType, EnumType FaultKeyEnumType>
ElijahStateFramework<StateDataType, PersistentKeyEnumType, FaultKeyEnumType>::~ElijahStateFramework()
{
  delete persistent_data_storage;
}

template <typename StateDataType, EnumType PersistentKeyEnumType, EnumType FaultKeyEnumType>
PersistentDataStorage<PersistentKeyEnumType>* ElijahStateFramework<
  StateDataType, PersistentKeyEnumType, FaultKeyEnumType>::
get_persistent_data_storage() const
{
  return persistent_data_storage;
}

template <typename StateDataType, EnumType PersistentKeyEnumType, EnumType FaultKeyEnumType>
void ElijahStateFramework<StateDataType, PersistentKeyEnumType, FaultKeyEnumType>::check_for_commands()
{
  if (!stdio_usb_connected())
  {
    return;
  }

  critical_section_enter_blocking(&usb_cs);

  uint8_t command_id = 0xFF;
  int bytes_read = stdio_get_until(reinterpret_cast<char*>(&command_id), 1, delayed_by_ms(get_absolute_time(), 10));
  if (bytes_read != 1)
  {
    critical_section_exit(&usb_cs);
    return;
  }

  if (!registered_commands.contains(command_id))
  {
    critical_section_exit(&usb_cs);
    return;
  }
  const RegisteredCommand* command = &registered_commands[command_id];

  double double_arg;
  std::string str_arg;
  tm time_arg{};

  switch (command->get_input_type())
  {
  case CommandInputType::Double:
    {
      double value;
      bytes_read = stdio_get_until(reinterpret_cast<char*>(&value), sizeof(value),
                                   delayed_by_ms(get_absolute_time(), 10));
      critical_section_exit(&usb_cs);
      if (bytes_read != sizeof(value))
      {
        return;
      }
      break;
    }
  case CommandInputType::AlphaNumeric:
  case CommandInputType::String:
    {
      uint8_t str_size_buf[2];
      bytes_read = stdio_get_until(reinterpret_cast<char*>(str_size_buf), 2, delayed_by_ms(get_absolute_time(), 10));
      if (bytes_read != 2)
      {
        critical_section_exit(&usb_cs);
        return;
      }

      const uint16_t str_size = *reinterpret_cast<uint16_t*>(str_size_buf);
      char str_buff[str_size + 1];
      bytes_read = stdio_get_until(str_buff, str_size, delayed_by_ms(get_absolute_time(), 10));
      critical_section_exit(&usb_cs);

      if (bytes_read != str_size)
      {
        return;
      }
      str_buff[str_size] = '\0';
      str_arg = std::string(str_buff);

      break;
    }
  case CommandInputType::Time:
    critical_section_exit(&usb_cs);
  // TODO time input
    break;
  case CommandInputType::None:
  default:
    critical_section_exit(&usb_cs);
    break;
  }

  std::visit(overloaded{
               [](const std::function<void()>& cb) { cb(); },
               [&double_arg](const std::function<void(double)>& cb) { cb(double_arg); },
               [&str_arg](const std::function<void(std::string)>& cb) { cb(str_arg); },
               [&time_arg](const std::function<void(tm)>& cb) { cb(time_arg); },
             }, command->get_callback());
}

template <typename StateDataType, EnumType PersistentKeyEnumType, EnumType FaultKeyEnumType>
void ElijahStateFramework<StateDataType, PersistentKeyEnumType, FaultKeyEnumType>::state_changed(
  const StateDataType& new_state)
{
  const size_t total_encoded_packet_size = encoded_state_size + 1;
  const auto encoded_output_packet = new uint8_t[total_encoded_packet_size];
  encoded_output_packet[0] = static_cast<uint8_t>(OutputPacket::StateUpdate);
  encode_state(encoded_output_packet + 1, new_state);

  shared_mutex_enter_blocking_exclusive(&state_history_smtx);

  if (state_history.size() >= state_history_size)
  {
    state_history.pop_back();
  }
  state_history.push_front(new_state);
  shared_mutex_exit_exclusive(&state_history_smtx);

  if (stdio_usb_connected())
  {
    critical_section_enter_blocking(&usb_cs);
    write_to_serial(encoded_output_packet, total_encoded_packet_size);
    critical_section_exit(&usb_cs);
  }

  lock_logger();
  if (logger)
  {
    logger->log_data(encoded_output_packet, total_encoded_packet_size);
  }
  release_logger();

  delete [] encoded_output_packet;
}

template <typename StateDataType, EnumType PersistentKeyEnumType, EnumType FaultKeyEnumType>
void ElijahStateFramework<StateDataType, PersistentKeyEnumType, FaultKeyEnumType>::lock_state_history()
{
  shared_mutex_enter_blocking_shared(&state_history_smtx);
}

template <typename StateDataType, EnumType PersistentKeyEnumType, EnumType FaultKeyEnumType>
void ElijahStateFramework<StateDataType, PersistentKeyEnumType, FaultKeyEnumType>::release_state_history()
{
  shared_mutex_exit_shared(&state_history_smtx);
}

template <typename StateDataType, EnumType PersistentKeyEnumType, EnumType FaultKeyEnumType>
const std::deque<StateDataType>& ElijahStateFramework<StateDataType, PersistentKeyEnumType, FaultKeyEnumType>::
get_state_history() const
{
  return state_history;
}

template <typename StateDataType, EnumType PersistentKeyEnumType, EnumType FaultKeyEnumType>
void ElijahStateFramework<StateDataType, PersistentKeyEnumType, FaultKeyEnumType>::set_fault(FaultKeyEnumType fault_key,
  const bool fault_state)
{
  set_fault(fault_key, fault_state, "");
}

template <typename StateDataType, EnumType PersistentKeyEnumType, EnumType FaultKeyEnumType>
void ElijahStateFramework<StateDataType, PersistentKeyEnumType, FaultKeyEnumType>::set_fault(FaultKeyEnumType fault_key,
  bool fault_state, const std::string& message)
{
  uint8_t fault_bit;
  bool did_fault_change;
  const uint32_t faults = fault_manager->set_fault_status(fault_key, fault_state, fault_bit, did_fault_change);

  if (!did_fault_change)
  {
    return;
  }

  const size_t encoded_size = 2 * sizeof(uint8_t) /* output packet, fault_bit */ + sizeof(uint32_t) + message.size() +
    1;
  uint8_t encoded_data[encoded_size];

  encoded_data[0] = static_cast<uint8_t>(OutputPacket::FaultsChanged);
  encoded_data[1] = fault_bit;
  memcpy(encoded_data + 2 * sizeof(uint8_t), &faults, sizeof(uint32_t));
  memcpy(encoded_data + 2 * sizeof(uint8_t) + sizeof(uint32_t), message.c_str(), message.size() + 1);

  critical_section_enter_blocking(&usb_cs);
  write_to_serial(encoded_data, encoded_size);
  critical_section_exit(&usb_cs);

  lock_logger();
  if (logger)
  {
    logger->log_data(encoded_data, encoded_size);
  }
  release_logger();

  if (faults > 0)
  {
    // TODO change status LED?
  }
}

template <typename StateDataType, EnumType PersistentKeyEnumType, EnumType FaultKeyEnumType>
bool ElijahStateFramework<StateDataType, PersistentKeyEnumType, FaultKeyEnumType>::is_faulted(
  FaultKeyEnumType fault_key)
{
  return fault_manager->is_faulted(fault_key);
}

template <typename StateDataType, EnumType PersistentKeyEnumType, EnumType FaultKeyEnumType>
void ElijahStateFramework<StateDataType, PersistentKeyEnumType, FaultKeyEnumType>::encode_state(void* encode_dest,
  const StateDataType& state)
{
  encode_state(encode_dest, state, state_seq, false);
  state_seq++;
}

template <typename StateDataType, EnumType PersistentKeyEnumType, EnumType FaultKeyEnumType>
const std::string& ElijahStateFramework<StateDataType, PersistentKeyEnumType,
                                        FaultKeyEnumType>::get_application_name() const
{
  return application_name;
}

template <typename StateDataType, EnumType PersistentKeyEnumType, EnumType FaultKeyEnumType>
void ElijahStateFramework<StateDataType, PersistentKeyEnumType, FaultKeyEnumType>::log_message(
  const std::string& message,
  LogLevel log_level)
{
  size_t encoded_len;
  const std::unique_ptr<uint8_t[]> encoded_message = encode_log_message(message, log_level, encoded_len);

  if (stdio_usb_connected())
  {
    critical_section_enter_blocking(&usb_cs);
    write_to_serial(encoded_message.get(), encoded_len);
    critical_section_exit(&usb_cs);
  }

  if (log_level != LogLevel::Debug && logger)
  {
    logger->log_data(encoded_message.get(), encoded_len);
  }
}

template <typename StateDataType, EnumType PersistentKeyEnumType, EnumType FaultKeyEnumType>
StateFrameworkLogger* ElijahStateFramework<StateDataType, PersistentKeyEnumType, FaultKeyEnumType>::get_logger() const
{
  return logger;
}

template <typename StateDataType, EnumType PersistentKeyEnumType, EnumType FaultKeyEnumType>
void ElijahStateFramework<StateDataType, PersistentKeyEnumType, FaultKeyEnumType>::lock_logger()
{
  shared_mutex_enter_blocking_shared(&logger_smtx);
}

template <typename StateDataType, EnumType PersistentKeyEnumType, EnumType FaultKeyEnumType>
void ElijahStateFramework<StateDataType, PersistentKeyEnumType, FaultKeyEnumType>::release_logger()
{
  shared_mutex_enter_blocking_shared(&logger_smtx);
}

template <typename StateDataType, EnumType PersistentKeyEnumType, EnumType FaultKeyEnumType>
void ElijahStateFramework<StateDataType, PersistentKeyEnumType, FaultKeyEnumType>::log_serial_only(
  const std::string& message)
{
  if (!stdio_usb_connected())
  {
    return;
  }

  size_t encoded_len;
  std::unique_ptr<uint8_t[]> encoded_message = encode_log_message(message, LogLevel::SerialOnly, encoded_len);

  critical_section_enter_blocking(&usb_cs);
  write_to_serial(encoded_message.get(), encoded_len);
  critical_section_exit(&usb_cs);
}

template <typename StateDataType, EnumType PersistentKeyEnumType, EnumType FaultKeyEnumType>
void ElijahStateFramework<StateDataType, PersistentKeyEnumType, FaultKeyEnumType>::register_command(
  const std::string& command,
  std::function<void()>&& callback)
{
  register_command(command, CommandInputType::None, callback);
}

template <typename StateDataType, EnumType PersistentKeyEnumType, EnumType FaultKeyEnumType>
void ElijahStateFramework<StateDataType, PersistentKeyEnumType, FaultKeyEnumType>::register_command(
  const std::string& command,
  std::function<void(double)>&& callback)
{
  register_command(command, CommandInputType::Double, callback);
}

template <typename StateDataType, EnumType PersistentKeyEnumType, EnumType FaultKeyEnumType>
void ElijahStateFramework<StateDataType, PersistentKeyEnumType, FaultKeyEnumType>::register_command(
  const std::string& command,
  const bool is_alphanumeric,
  std::function<void(std::string)>&&
  callback)
{
  register_command(command, is_alphanumeric ? CommandInputType::AlphaNumeric : CommandInputType::String, callback);
}

template <typename StateDataType, EnumType PersistentKeyEnumType, EnumType FaultKeyEnumType>
void ElijahStateFramework<StateDataType, PersistentKeyEnumType, FaultKeyEnumType>::register_command(
  const std::string& command,
  std::function<void(tm)>&& callback)
{
  register_command(command, CommandInputType::Time, callback);
}

template <typename StateDataType, EnumType PersistentKeyEnumType, EnumType FaultKeyEnumType>
void ElijahStateFramework<StateDataType, PersistentKeyEnumType, FaultKeyEnumType>::register_data_variable(
  const std::string& display_name, const std::string& display_unit, const size_t offset, const DataType data_type)
{
  const uint8_t variable_id = variable_id_counter;
  variable_id_counter++;

  variable_definitions[variable_id] = VariableDefinition(variable_id, display_name,
                                                         display_unit, offset, data_type);
}

template <typename StateDataType, EnumType PersistentKeyEnumType, EnumType FaultKeyEnumType>
void ElijahStateFramework<StateDataType, PersistentKeyEnumType, FaultKeyEnumType>::register_fault(FaultKeyEnumType key,
  std::string fault_name, CommunicationChannel communication_channel)
{
  fault_manager->register_fault(key, fault_name, communication_channel);
}

template <typename StateDataType, EnumType PersistentKeyEnumType, EnumType FaultKeyEnumType>
void ElijahStateFramework<StateDataType, PersistentKeyEnumType, FaultKeyEnumType>::set_encoded_state_size(
  const size_t encoded_data_size)
{
  is_size_calculated = true;
  encoded_state_size = encoded_data_size;
}

template <typename StateDataType, EnumType PersistentKeyEnumType, EnumType FaultKeyEnumType>
void ElijahStateFramework<StateDataType, PersistentKeyEnumType, FaultKeyEnumType>::finish_construction()
{
  logger = new StateFrameworkLogger(persistent_data_storage->get_string(launch_key));

  StateDataType collection_data;
  encode_state(nullptr, collection_data, 0, true);

  if (logger->is_new_file())
  {
    send_framework_metadata(true);
  }
  else
  {
    constexpr auto restart_marker = static_cast<uint8_t>(OutputPacket::DeviceRestartMarker);
    logger->log_data(&restart_marker, sizeof(uint8_t));
  }

  persistent_data_storage->lock_active_data();
  send_persistent_state(persistent_data_storage->get_active_data_loc(), persistent_data_storage->get_total_byte_size());
  persistent_data_storage->release_active_data();
}

template <typename StateDataType, EnumType PersistentKeyEnumType, EnumType FaultKeyEnumType>
void ElijahStateFramework<StateDataType, PersistentKeyEnumType, FaultKeyEnumType>::initialize_communication()
{
  stdio_init_all();
  critical_section_init(&usb_cs);
}

template <typename StateDataType, EnumType PersistentKeyEnumType, EnumType FaultKeyEnumType>
void ElijahStateFramework<StateDataType, PersistentKeyEnumType, FaultKeyEnumType>::write_to_serial(
  const uint8_t* write_data, const size_t write_len)
{
  write_to_serial(write_data, write_len, true);
}

template <typename StateDataType, EnumType PersistentKeyEnumType, EnumType FaultKeyEnumType>
void ElijahStateFramework<StateDataType, PersistentKeyEnumType, FaultKeyEnumType>::write_to_serial(
  const uint8_t* packet_data,
  const size_t packet_len, const bool flush)
{
  stdio_put_string(reinterpret_cast<const char*>(packet_data), static_cast<int>(packet_len), false, false);
  if (flush)
  {
    stdio_flush();
  }
}

template <typename StateDataType, EnumType PersistentKeyEnumType, EnumType FaultKeyEnumType>
std::unique_ptr<uint8_t[]> ElijahStateFramework<StateDataType, PersistentKeyEnumType,
                                                FaultKeyEnumType>::encode_log_message(
  const std::string& message, LogLevel log_level, size_t& encoded_len)
{
  std::string send_message = message;

  // TODO, do not hardcode, should be less than write buff len for logger
  if (message.size() > 1000)
  {
    send_message = "Message too large... ";
    send_message += message.substr(0, 1000 - send_message.size());
  }

  encoded_len = sizeof(uint8_t) /* packet id */ + sizeof(uint8_t) /* log level */ + sizeof(uint16_t)
    /* message length */ + send_message.size();

  std::unique_ptr<uint8_t[]> encoded_message(new uint8_t[encoded_len]{
    static_cast<uint8_t>(OutputPacket::LogMessage), static_cast<uint8_t>(log_level)
  });

  *reinterpret_cast<uint16_t*>(encoded_message.get() + 2) = static_cast<uint16_t>(send_message.size());
  memcpy(encoded_message.get() + (2 * sizeof(uint8_t) + sizeof(uint16_t)), send_message.c_str(), send_message.size());

  return encoded_message;
}

template <typename StateDataType, EnumType PersistentKeyEnumType, EnumType FaultKeyEnumType>
void ElijahStateFramework<StateDataType, PersistentKeyEnumType, FaultKeyEnumType>::register_command(
  const std::string& command,
  const CommandInputType command_input,
  command_callback_t callback)
{
  const uint8_t new_id = command_id_counter++;
  registered_commands[new_id] = RegisteredCommand(new_id, command, command_input, std::move(callback));
}

template <typename StateDataType, EnumType PersistentKeyEnumType, EnumType FaultKeyEnumType>
void ElijahStateFramework<StateDataType, PersistentKeyEnumType, FaultKeyEnumType>::send_framework_metadata(
  const bool write_to_file)
{
  if (!write_to_file && !stdio_usb_connected())
  {
    return;
  }

  critical_section_enter_blocking(&usb_cs);
  if (write_to_file && logger)
  {
    lock_logger();
    const auto packet_id = static_cast<uint8_t>(OutputPacket::Metadata);
    logger->log_data(&packet_id, sizeof(packet_id));
  }

  auto segment_id = static_cast<uint8_t>(MetadataSegment::ApplicationName);
  const size_t initial_size = sizeof(FRAMEWORK_TAG) + sizeof(uint8_t) + application_name.size() + 1;
  auto initial_data = new uint8_t[initial_size];

  *reinterpret_cast<uint64_t*>(initial_data) = FRAMEWORK_TAG;
  initial_data[sizeof(FRAMEWORK_TAG)] = segment_id;
  memcpy(initial_data + sizeof(FRAMEWORK_TAG) + sizeof(segment_id), application_name.c_str(),
         application_name.size() + 1);
  write_to_serial(initial_data, initial_size, false);
  if (write_to_file && logger)
  {
    logger->log_data(initial_data + sizeof(FRAMEWORK_TAG), sizeof(uint8_t) + application_name.size() + 1);
  }

  const uint8_t command_count = registered_commands.size();
  if (command_count > 0)
  {
    segment_id = static_cast<uint8_t>(MetadataSegment::Commands);
    const uint8_t segment_header[2] = {segment_id, command_count};
    write_to_serial(segment_header, 2, false);

    for (const auto& command : std::views::values(registered_commands))
    {
      size_t encoded_size;
      std::unique_ptr<uint8_t[]> encoded = command.encode_command(encoded_size);
      write_to_serial(encoded.get(), encoded_size, false);
    }
  }

  const uint8_t var_count = variable_definitions.size();
  if (var_count > 0)
  {
    segment_id = static_cast<uint8_t>(MetadataSegment::VariableDefinitions);
    const uint8_t segment_header[3] = {segment_id, var_count, static_cast<uint8_t>(sizeof(size_t))};
    write_to_serial(segment_header, 3, false);
    if (write_to_file && logger)
    {
      logger->log_data(segment_header, 3);
    }

    for (const auto& var_def : std::views::values(variable_definitions))
    {
      size_t encoded_size;
      std::unique_ptr<uint8_t[]> encoded = var_def.encode_var(encoded_size);
      write_to_serial(encoded.get(), encoded_size, false);
      if (write_to_file && logger)
      {
        logger->log_data(encoded.get(), encoded_size);
      }
    }
  }

  // Always has launch key
  segment_id = static_cast<uint8_t>(MetadataSegment::PersistentStorageEntries);
  const uint8_t persistent_data_segment_header[3] = {
    segment_id, static_cast<uint8_t>(persistent_data_storage->get_entry_count()), static_cast<uint8_t>(sizeof(size_t))
  };
  write_to_serial(persistent_data_segment_header, 3, false);
  if (write_to_file && logger)
  {
    logger->log_data(persistent_data_segment_header, 3);
  }

  size_t encoded_size;
  std::unique_ptr<uint8_t[]> encoded_data = persistent_data_storage->encode_all_entries(encoded_size);

  // We need to flush here so that we can dump persistent storage
  write_to_serial(encoded_data.get(), encoded_size);
  if (write_to_file && logger)
  {
    logger->log_data(encoded_data.get(), encoded_size);
    logger->flush_log();
  }

  persistent_data_storage->lock_active_data();
  write_to_serial(static_cast<uint8_t*>(persistent_data_storage->get_active_data_loc()),
                  persistent_data_storage->get_total_byte_size());
  if (write_to_file && logger)
  {
    logger->log_data(static_cast<uint8_t*>(persistent_data_storage->get_active_data_loc()),
                     persistent_data_storage->get_total_byte_size());
  }
  persistent_data_storage->release_active_data();

  // Always has communication channels, so we can skip size checks
  segment_id = static_cast<uint8_t>(MetadataSegment::FaultInformation);
  const uint32_t all_faults = fault_manager->get_all_faults();

  constexpr size_t header_len = 2 + sizeof(uint32_t);
  uint8_t fault_segment_header[header_len] = {
    segment_id, static_cast<uint8_t>(fault_manager->get_fault_count())
  };
  memcpy(fault_segment_header + 2, &all_faults, sizeof(uint32_t));

  write_to_serial(fault_segment_header, header_len, false);
  if (write_to_file && logger)
  {
    logger->log_data(fault_segment_header, 2);
  }

  encoded_data = fault_manager->encode_all_faults(encoded_size);
  write_to_serial(encoded_data.get(), encoded_size, false);
  if (write_to_file && logger)
  {
    logger->log_data(encoded_data.get(), encoded_size);
  }

  segment_id = static_cast<uint8_t>(MetadataSegment::MetadataEnd);
  write_to_serial(&segment_id, 1);
  if (write_to_file && logger)
  {
    logger->log_data(&segment_id, 1);
    release_logger();
  }

  critical_section_exit(&usb_cs);
}

template <typename StateDataType, EnumType PersistentKeyEnumType, EnumType FaultKeyEnumType>
void ElijahStateFramework<StateDataType, PersistentKeyEnumType, FaultKeyEnumType>::send_persistent_state(
  const void* data, const size_t data_len)
{
  critical_section_enter_blocking(&usb_cs);
  const auto packet_id = static_cast<uint8_t>(OutputPacket::PersistentStateUpdate);

  // Data length will be parsed by pre-existing persistent data segments
  write_to_serial(&packet_id, 1, false);
  write_to_serial(static_cast<const uint8_t*>(data), data_len);

  critical_section_exit(&usb_cs);

  if (logger)
  {
    lock_logger();
    logger->log_data(&packet_id, 1);
    logger->log_data(static_cast<const uint8_t*>(data), data_len);
    logger->flush_log();
    release_logger();
  }
}
