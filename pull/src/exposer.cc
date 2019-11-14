#include "prometheus/exposer.h"

#include <chrono>
#include <string>
#include <thread>

#include "prometheus/client_metric.h"

#include "CivetServer.h"
#include "handler.h"

namespace prometheus {


void ProxygenServer::addHandler(const std::string& url, handler_gen_type&& handler) {
  handlersGen_->emplace(url, handler);
}

void ProxygenServer::removeHandler(const std::string& url) {
  // Do nothing in proxygen
}

Exposer::Exposer(Server* s, const std::string& uri,
                 const std::size_t num_threads)
    : server_(s),
      exposer_registry_(std::make_shared<Registry>()),
      uri_(uri) {
  RegisterCollectable(exposer_registry_);
  server_->addHandler(uri, [this](){
    return new detail::MetricsHandler(collectables_, *exposer_registry_);
  });
}

Exposer::~Exposer() { server_->removeHandler(uri_); }

void Exposer::RegisterCollectable(
    const std::weak_ptr<Collectable>& collectable) {
  collectables_.push_back(collectable);
}
}  // namespace prometheus
