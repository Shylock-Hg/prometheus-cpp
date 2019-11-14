#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include <proxygen/httpserver/HTTPServer.h>
#include <proxygen/httpserver/RequestHandler.h>

#include "prometheus/collectable.h"
#include "prometheus/detail/pull_export.h"
#include "prometheus/registry.h"

//class CivetServer;

namespace prometheus {

namespace detail {
class MetricsHandler;
}  // namespace detail


/// The monitor are only the attached system,
/// So these will not fetch any ownership of real Server/Handler, their lifetime
/// is managed by the Application self.
class Server;
class Handler : public proxygen::RequestHandler {
public:
  Handler() = default;
  ~Handler() = default;

  /// civet
  /// proxygen
};

// Must use the external Factory in Application
// For it's the terminal Factory
//class HandlerFactory : public proxygen::RequestHandlerFactory {
//public:
  //HandlerFactory() = default;
  //~HandlerFactory() = default;

  //// proxygen
//};

class Server {
public:
  Server() = default;
  virtual ~Server() = default;

  using handler_gen_type = std::function<Handler*()>;
  using handlers_gen_type = std::unordered_map<std::string, handler_gen_type>;

  virtual void addHandler(const std::string&, handler_gen_type&&) = 0;
  virtual void removeHandler(const std::string&) = 0;
};

//class ProxygenHandler final : public Handler {
//public:
  //ProxygenHandler(proxygen::RequestHandler* handler) : handler_(handler);
  ///// Do nothing
  ///// The handlers of proxygen is Created by the RequestHandlerFactory in fact
  //bool handleGet(Server* s);
//private:
  //proxygen::RequestHandler* handler_;
//};

class ProxygenServer final : public Server {
public:
  //ProxygenServer(proxygen::HTTPServer* s, proxygen::HTTPServerOptions* options)
    //: server_(s), options_(options) {}
  //using handler_gen_type = std::function<Handler*()>;
  //using handlers_gen_type = std::unordered_map<std::string, handler_gen_type>;

  ProxygenServer(handlers_gen_type* gens) : handlersGen_(gens) {}

  /// Add/Remove handlers to the real Sever
  /// Add the HandlerFactory to proxygen server options
  void addHandler(const std::string& url, handler_gen_type&& handler) override;
  void removeHandler(const std::string& url) override;

private:
  //proxygen::HTTPServer* server_;
  //proxygen::HTTPServerOptions* options_;
  handlers_gen_type* handlersGen_;
};

class PROMETHEUS_CPP_PULL_EXPORT Exposer {
 public:
  explicit Exposer(Server* s,
                   const std::string& uri = std::string("/metrics"),
                   const std::size_t num_threads = 2);
  ~Exposer();
  void RegisterCollectable(const std::weak_ptr<Collectable>& collectable);

 private:
  std::unique_ptr<Server> server_;
  std::vector<std::weak_ptr<Collectable>> collectables_;
  std::shared_ptr<Registry> exposer_registry_;
//  std::unique_ptr<detail::MetricsHandler> metrics_handler_;
  std::string uri_;
};

}  // namespace prometheus
