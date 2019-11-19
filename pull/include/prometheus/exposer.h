#pragma once

#include <atomic>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

//#include "CivetServer.h"
#include "proxygen/httpserver/HTTPServer.h"

#include "prometheus/collectable.h"
#include "prometheus/detail/pull_export.h"
#include "prometheus/registry.h"

class CivetServer;

namespace prometheus {

namespace detail {
class MetricsHandler;

class Server {
 public:
  Server() = default;
  virtual ~Server() = default;

  virtual void start(
      std::string path,
      const std::vector<std::weak_ptr<Collectable>>& collectables,
      Registry& registry) = 0;
};

/*
class CivetServerImpl final : public Server {
public:
  CivetServerImpl(const std::string& bind, const std::size_t num_threads);
  ~CivetServerImpl();

  void start(std::string path, const std::vector<std::weak_ptr<Collectable>>&
collectables, Registry& registry) override;

private:
  std::unique_ptr<MetricsHandler> metrics_handler_;
  CivetServer server_;
};
*/

class ProxygenServerImpl final : public Server {
 public:
  ProxygenServerImpl(std::vector<proxygen::HTTPServer::IPConfig> address);
  ~ProxygenServerImpl();

  void start(std::string path,
             const std::vector<std::weak_ptr<Collectable>>& collectables,
             Registry& registry) override;

 private:
  class HandlerFactory final : public proxygen::RequestHandlerFactory {
   public:
    HandlerFactory(const std::vector<std::weak_ptr<Collectable>>* collectables,
                   Registry* registry)
        : collectables_(collectables), registry_(registry) {}
    ~HandlerFactory() = default;

    void onServerStart(folly::EventBase*) noexcept override {}

    void onServerStop() noexcept override {}

    proxygen::RequestHandler* onRequest(
        proxygen::RequestHandler*,
        proxygen::HTTPMessage* msg) noexcept override;

   private:
    const std::vector<std::weak_ptr<Collectable>>* collectables_;
    Registry* registry_;
  };

  std::vector<proxygen::HTTPServer::IPConfig> address_;
  std::unique_ptr<proxygen::HTTPServer> server_;
  std::thread poller_;
};

class ProxygenRefServerImpl final : public Server {
 public:
  using HandlerGen =
      std::unordered_map<std::string,
                         std::function<proxygen::RequestHandler*()>>;
  ProxygenRefServerImpl(HandlerGen& gens) : gens_(gens) {}
  ~ProxygenRefServerImpl() = default;

  void start(std::string path,
             const std::vector<std::weak_ptr<Collectable>>& collectables,
             Registry& registry) override;

 private:
  HandlerGen& gens_;
};

}  // namespace detail

class PROMETHEUS_CPP_PULL_EXPORT Exposer {
 public:
  explicit Exposer(detail::Server* s,
                   const std::string& uri = std::string("/metrics"));
  ~Exposer();
  void RegisterCollectable(const std::weak_ptr<Collectable>& collectable);

 private:
  // Not take the ownership for this is a attached system
  // Manage the lifetime by application self
  detail::Server* server_;
  std::vector<std::weak_ptr<Collectable>> collectables_;
  std::shared_ptr<Registry> exposer_registry_;
  std::string uri_;
};

}  // namespace prometheus
