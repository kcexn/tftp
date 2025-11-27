/* Copyright (C) 2025 Kevin Exton (kevin.exton@pm.me)
 *
 * tftp is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * tftp is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with tftp.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "tftp/detail/argument_parser.hpp"
#include "tftp/tftp_client.hpp"

#include <algorithm>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <string>

using namespace net::service;
using namespace tftp;

static constexpr std::string PORT = "69";

struct config {
  /** @brief tftp method enum. */
  enum method_t : std::uint8_t { GET, PUT };
  /** @brief hostname of tftp server. */
  std::string hostname;
  /** @brief port number of tftp server, 69 by default. */
  std::string port = PORT;
  /** @brief local path for tftp operation. */
  std::filesystem::path local;
  /** @brief remote path for tftp operation. Should be absolute path. */
  std::filesystem::path remote;
  /** @brief tftp method. */
  method_t method;
  /** @brief tftp operation mode. */
  messages::mode_t mode;
};

static auto print_usage(const char *program_name) -> void
{
  std::cerr << "Usage: " << program_name << " [OPTIONS] get <remote> <local>\n"
            << "       " << program_name << " [OPTIONS] put <local> <remote>\n"
            << "Arguments:\n"
            << "  get <remote> <local>    Download remote file to local path\n"
            << "  put <local> <remote>    Upload local file to remote path\n"
            << "\nOptions:\n"
            << "  -h, --help              Display this help message\n"
            << "  -H, --host=<host[:port]> TFTP server hostname:port "
               "(required, default port: 69)\n"
            << "  --mode=<netascii|octet|mail> Transfer mode (default: octet)\n";
}

static auto to_lowercase(std::string_view input) -> std::string
{
  auto result = std::string(input);
  std::ranges::transform(result, result.begin(),
                         [](unsigned char chr) { return std::tolower(chr); });
  return result;
}

static auto parse_host_port(std::string_view host_str, std::string &hostname,
                            std::string &port) -> bool
{
  auto colon_pos = host_str.find(':');
  if (colon_pos == std::string_view::npos)
  {
    hostname = host_str;
    return true;
  }

  hostname = host_str.substr(0, colon_pos);
  port = host_str.substr(colon_pos + 1);

  if (hostname.empty() || port.empty())
  {
    std::cerr << "Error: Invalid host:port format\n";
    return false;
  }

  return true;
}

static auto
parse_method(std::string_view method_str) -> std::optional<config::method_t>
{
  auto lower = to_lowercase(method_str);
  if (lower == "get")
    return config::GET;
  if (lower == "put")
    return config::PUT;
  return std::nullopt;
}

static auto
parse_mode(std::string_view mode_str) -> std::optional<messages::mode_t>
{
  auto lower = to_lowercase(mode_str);
  if (lower == "netascii")
    return messages::NETASCII;
  if (lower == "octet")
    return messages::OCTET;
  if (lower == "mail")
    return messages::MAIL;
  return std::nullopt;
}

static auto handle_option(const tftp::detail::argument_parser::option &opt,
                          config &conf, bool &has_hostname,
                          const char *program_name) -> bool
{
  if (opt.flag == "-h" || opt.flag == "--help")
  {
    print_usage(program_name);
    return false;
  }

  if (opt.flag == "-H" || opt.flag == "--host")
  {
    if (opt.value.empty())
    {
      std::cerr << "Error: --host requires a value\n";
      return false;
    }
    if (!parse_host_port(opt.value, conf.hostname, conf.port))
      return false;
    has_hostname = true;
    return true;
  }

  if (opt.flag == "--mode")
  {
    if (opt.value.empty())
    {
      std::cerr << "Error: --mode requires a value\n";
      return false;
    }

    auto mode = parse_mode(opt.value);
    if (!mode)
    {
      std::cerr << "Error: --mode must be 'netascii', 'octet', or 'mail'\n";
      return false;
    }
    conf.mode = *mode;
    return true;
  }

  if (!opt.flag.empty())
  {
    std::cerr << "Error: Unknown option: " << opt.flag << "\n";
    return false;
  }

  return true;
}

static auto handle_positional(std::string_view value, std::size_t position,
                              config &conf, std::string &first_arg,
                              std::string &second_arg) -> bool
{
  if (position == 0)
  {
    auto method = parse_method(value);
    if (!method)
    {
      std::cerr << "Error: method must be 'get' or 'put'\n";
      return false;
    }
    conf.method = *method;
    return true;
  }

  if (position == 1)
  {
    first_arg = value;
    return true;
  }

  if (position == 2)
  {
    second_arg = value;
    return true;
  }

  std::cerr << "Error: Too many positional arguments\n";
  return false;
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-c-arrays)
static auto parse_arguments(int argc, char *argv[]) -> std::optional<config>
{
  using namespace tftp::detail;

  const char *program_name = argv[0];

  auto conf = config{};
  conf.port = PORT;
  conf.mode = messages::OCTET;

  bool has_hostname = false;
  std::size_t positional_count = 0;
  std::string first_arg;
  std::string second_arg;

  for (const auto &opt : argument_parser::parse(argc, argv))
  {
    if (!opt.flag.empty())
    {
      if (!handle_option(opt, conf, has_hostname, program_name))
      {
        return std::nullopt;
      }
    }
    else if (!opt.value.empty())
    {
      if (!handle_positional(opt.value, positional_count, conf, first_arg,
                             second_arg))
      {
        return std::nullopt;
      }
      ++positional_count;
    }
  }

  if (positional_count < 3)
  {
    std::cerr << "Error: Missing required positional arguments\n";
    print_usage(argv[0]);
    return std::nullopt;
  }

  if (!has_hostname)
  {
    std::cerr << "Error: Missing required --host option\n";
    print_usage(argv[0]);
    return std::nullopt;
  }

  if (conf.method == config::GET)
  {
    conf.remote = first_arg;
    conf.local = second_arg;
  }
  else
  {
    conf.local = first_arg;
    conf.remote = second_arg;
  }

  return conf;
}

static auto put_file(const config &conf,
                     const client_manager::client_t &client) -> void
{
  using namespace stdexec;

  const auto &host = conf.hostname;
  const auto &port = conf.port;
  const auto &local = conf.local;
  const auto &remote = conf.remote;
  const auto &mode = conf.mode;

  sender auto put_file = client.connect(host, port) | let_value([&](auto addr) {
                           return client.put(addr, local, remote, mode);
                         });

  auto [status] = sync_wait(std::move(put_file)).value();

  auto &[error, message] = status;
  if (error || !message.empty())
    std::cerr << std::format("{} {}\n", error, message);
}

static auto get_file(const config &conf,
                     const client_manager::client_t &client) -> void
{
  using namespace stdexec;

  const auto &host = conf.hostname;
  const auto &port = conf.port;
  const auto &local = conf.local;
  const auto &remote = conf.remote;
  const auto &mode = conf.mode;

  sender auto get_file = client.connect(host, port) | let_value([&](auto addr) {
                           return client.get(addr, remote, local, mode);
                         });

  auto [status] = sync_wait(std::move(get_file)).value();

  auto &[error, message] = status;
  if (error || !message.empty())
    std::cerr << std::format("{} {}\n", error, message);
}

auto main(int argc, char *argv[]) -> int
{
  using namespace stdexec;

  auto opts = parse_arguments(argc, argv);
  if (!opts)
    return 1;

  auto &conf = *opts;
  auto manager = client_manager();
  auto client = manager.make_client();

  switch (conf.method)
  {
    case config::PUT:
      put_file(conf, client);
      break;

    case config::GET:
      get_file(conf, client);
      break;

    default:
      break;
  }

  return 0;
}
