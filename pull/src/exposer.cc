#include "prometheus/exposer.h"

#include <chrono>
#include <string>
#include <thread>

#include "prometheus/client_metric.h"

//#include "CivetServer.h"
#include "handler.h"

namespace prometheus {

Exposer::Exposer(detail::Server* s, const std::string& uri)
    : server_(s),
      exposer_registry_(std::make_shared<Registry>()),
      uri_(uri) {
  RegisterCollectable(exposer_registry_);
  server_->start(uri, collectables_, *exposer_registry_);
}

Exposer::~Exposer() { /*server_->removeHandler(uri_);*/ }

void Exposer::RegisterCollectable(
    const std::weak_ptr<Collectable>& collectable) {
  collectables_.push_back(collectable);
}

namespace detail {

/*
  CivetServerImpl::~CivetServerImpl() = default;

  CivetServerImpl::CivetServerImpl(const std::string& bind, const std::size_t num_threads) :
   server_(
    std::vector<std::string>{
      "listening_ports", bind, "num_threads",
      std::to_string(num_threads)}
  ) , metrics_handler_(nullptr) {}

  void CivetServerImpl::start(std::string path, const std::vector<std::weak_ptr<Collectable>>& collectables,
                 Registry& registry) {
    metrics_handler_.reset(new MetricsHandler(collectables, registry));
    server_.addHandler(path, metrics_handler_.get());
  }
*/


  ProxygenServerImpl::ProxygenServerImpl(std::vector<proxygen::HTTPServer::IPConfig> address) : address_(address), server_(nullptr) {}
  ProxygenServerImpl::~ProxygenServerImpl() {
    server_->stop();
    poller_.join();
  };

  void ProxygenServerImpl::start(std::string path, const std::vector<std::weak_ptr<Collectable>>& collectables,
                 Registry& registry) {
    proxygen::HTTPServerOptions options;
    options.threads = static_cast<size_t>(2);
    options.idleTimeout = std::chrono::milliseconds(60000);
    options.enableContentCompression = false;
    options.handlerFactories = proxygen::RequestHandlerChain()
      .addThen<HandlerFactory>(&collectables, &registry)
      .build();
    options.h2cEnabled = true;
    server_.reset(new proxygen::HTTPServer(std::move(options)));

    server_->bind(address_);
    poller_ = std::thread([this]() {
      server_->start();
    });
  }

  proxygen::RequestHandler* ProxygenServerImpl::HandlerFactory::onRequest(proxygen::RequestHandler*,
                                      proxygen::HTTPMessage* msg) noexcept {
      std::string path = msg->getPath();
      if (path == "/metrics") {
        return dynamic_cast<proxygen::RequestHandler*> (new detail::MetricsHandler(*collectables_, *registry_));
      }

      // Unknown url path
//        return new NotFoundHandler();
      return nullptr;
  }
}


}  // namespace prometheus
