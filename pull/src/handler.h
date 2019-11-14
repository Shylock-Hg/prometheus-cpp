#pragma once

#include <memory>
#include <vector>

//#include "CivetServer.h"

#include "prometheus/exposer.h"

#include "prometheus/counter.h"
#include "prometheus/registry.h"
#include "prometheus/summary.h"

namespace prometheus {
namespace detail {
class MetricsHandler : public Handler {
 public:
  MetricsHandler(const std::vector<std::weak_ptr<Collectable>>& collectables,
                 Registry& registry);

//  bool handleGet(CivetServer* server, struct mg_connection* conn) override;

  // Proxygen
  void onRequest(std::unique_ptr<proxygen::HTTPMessage> headers) noexcept override;

  void onBody(std::unique_ptr<folly::IOBuf> body) noexcept override;

  void onUpgrade(proxygen::UpgradeProtocol prot) noexcept override;

  void onEOM() noexcept override;

  void requestComplete() noexcept override;

  void onError(proxygen::ProxygenError err) noexcept override;

 private:
  std::vector<MetricFamily> CollectMetrics() const;

  const std::vector<std::weak_ptr<Collectable>>& collectables_;
  Family<Counter>& bytes_transferred_family_;
  Counter& bytes_transferred_;
  Family<Counter>& num_scrapes_family_;
  Counter& num_scrapes_;
  Family<Summary>& request_latencies_family_;
  Summary& request_latencies_;
  int err_{0};
  std::string err_msg_;
};
}  // namespace detail
}  // namespace prometheus
