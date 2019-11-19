#include <chrono>
#include <map>
#include <memory>
#include <string>
#include <thread>

#include <prometheus/counter.h>
#include <prometheus/exposer.h>
#include <prometheus/registry.h>

#include <folly/SocketAddress.h>

using namespace prometheus;

class WebServiceHandlerFactory : public proxygen::RequestHandlerFactory {
 public:
  explicit WebServiceHandlerFactory(
      detail::ProxygenRefServerImpl::HandlerGen&& genMap)
      : handlerGenMap_(genMap) {}

  void onServerStart(folly::EventBase*) noexcept override {}

  void onServerStop() noexcept override {}

  proxygen::RequestHandler* onRequest(
      proxygen::RequestHandler*, proxygen::HTTPMessage* msg) noexcept override {
    std::string path = msg->getPath();
    if (path == "/") {
      path = "/status";
    }
    //        VLOG(2) << "Got request \"" << path << "\"";
    auto it = handlerGenMap_.find(path);
    if (it != handlerGenMap_.end()) {
      return it->second();
    }

    // Unknown url path
    //        return new NotFoundHandler();
    return nullptr;
  }

 private:
  detail::ProxygenRefServerImpl::HandlerGen handlerGenMap_;
};

int main() {
  detail::ProxygenRefServerImpl::HandlerGen handlerGenMap;

  detail::ProxygenRefServerImpl s(handlerGenMap);

  Exposer exposer{&s};

  proxygen::HTTPServerOptions options;
  options.threads = static_cast<size_t>(2);
  options.idleTimeout = std::chrono::milliseconds(60000);
  options.enableContentCompression = false;
  options.handlerFactories =
      proxygen::RequestHandlerChain()
          .addThen<WebServiceHandlerFactory>(std::move(handlerGenMap))
          .build();
  options.h2cEnabled = true;

  proxygen::HTTPServer server(std::move(options));

  std::vector<proxygen::HTTPServer::IPConfig> IPs = {
      {folly::SocketAddress("localhost", 8080, true),
       proxygen::HTTPServer::Protocol::HTTP},
      {folly::SocketAddress("localhost", 8082, true),
       proxygen::HTTPServer::Protocol::HTTP2},
  };

  server.bind(IPs);

  // Start HTTPServer mainloop in a separate thread
  std::thread t([&]() { server.start(); });

  // create a metrics registry with component=main labels applied to all its
  // metrics
  auto registry = std::make_shared<Registry>();

  // add a new counter family to the registry (families combine values with the
  // same name, but distinct label dimensions)
  auto& counter_family = BuildCounter()
                             .Name("time_running_seconds_total")
                             .Help("How many seconds is this server running?")
                             .Labels({{"label", "value"}})
                             .Register(*registry);

  // add a counter to the metric family
  auto& second_counter = counter_family.Add(
      {{"another_label", "value"}, {"yet_another_label", "value"}});

  // ask the exposer to scrape the registry on incoming scrapes
  exposer.RegisterCollectable(registry);

  for (;;) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
    // increment the counter by one (second)
    second_counter.Increment();
  }

  t.join();

  return 0;
}
