#include "handler.h"
#include "prometheus/counter.h"
#include "prometheus/summary.h"

#include <cstring>
#include <iterator>

#ifdef HAVE_ZLIB
#include <zlib.h>
#endif

#include "prometheus/serializer.h"
#include "prometheus/text_serializer.h"

#include "proxygen/httpserver/ResponseBuilder.h"

namespace prometheus {
namespace detail {

MetricsHandler::MetricsHandler(
    const std::vector<std::weak_ptr<Collectable>>& collectables,
    Registry& registry)
    : collectables_(collectables),
      bytes_transferred_family_(
          BuildCounter()
              .Name("exposer_transferred_bytes_total")
              .Help("Transferred bytes to metrics services")
              .Register(registry)),
      bytes_transferred_(bytes_transferred_family_.Add({})),
      num_scrapes_family_(BuildCounter()
                              .Name("exposer_scrapes_total")
                              .Help("Number of times metrics were scraped")
                              .Register(registry)),
      num_scrapes_(num_scrapes_family_.Add({})),
      request_latencies_family_(
          BuildSummary()
              .Name("exposer_request_latencies")
              .Help("Latencies of serving scrape requests, in microseconds")
              .Register(registry)),
      request_latencies_(request_latencies_family_.Add(
          {}, Summary::Quantiles{{0.5, 0.05}, {0.9, 0.01}, {0.99, 0.001}})),
      err_(200),
      err_msg_("Ok") {}

/*
#ifdef HAVE_ZLIB
static bool IsEncodingAccepted(struct mg_connection* conn,
                               const char* encoding) {
  auto accept_encoding = mg_get_header(conn, "Accept-Encoding");
  if (!accept_encoding) {
    return false;
  }
  return std::strstr(accept_encoding, encoding) != nullptr;
}

static std::vector<Byte> GZipCompress(const std::string& input) {
  auto zs = z_stream{};
  auto windowSize = 16 + MAX_WBITS;
  auto memoryLevel = 9;

  if (deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, windowSize,
                   memoryLevel, Z_DEFAULT_STRATEGY) != Z_OK) {
    return {};
  }

  zs.next_in = (Bytef*)input.data();
  zs.avail_in = input.size();

  int ret;
  std::vector<Byte> output;
  output.reserve(input.size() / 2u);

  do {
    static const auto outputBytesPerRound = std::size_t{32768};

    zs.avail_out = outputBytesPerRound;
    output.resize(zs.total_out + zs.avail_out);
    zs.next_out = reinterpret_cast<Bytef*>(output.data() + zs.total_out);

    ret = deflate(&zs, Z_FINISH);

    output.resize(zs.total_out);
  } while (ret == Z_OK);

  deflateEnd(&zs);

  if (ret != Z_STREAM_END) {
    return {};
  }

  return output;
}
#endif

static std::size_t WriteResponse(struct mg_connection* conn,
                                 const std::string& body) {
  mg_printf(conn,
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n");

#ifdef HAVE_ZLIB
  auto acceptsGzip = IsEncodingAccepted(conn, "gzip");

  if (acceptsGzip) {
    auto compressed = GZipCompress(body);
    if (!compressed.empty()) {
      mg_printf(conn,
                "Content-Encoding: gzip\r\n"
                "Content-Length: %lu\r\n\r\n",
                static_cast<unsigned long>(compressed.size()));
      mg_write(conn, compressed.data(), compressed.size());
      return compressed.size();
    }
  }
#endif

  mg_printf(conn, "Content-Length: %lu\r\n\r\n",
            static_cast<unsigned long>(body.size()));
  mg_write(conn, body.data(), body.size());
  return body.size();
}

bool MetricsHandler::handleGet(CivetServer*, struct mg_connection* conn) {
  auto start_time_of_request = std::chrono::steady_clock::now();

  auto metrics = CollectMetrics();

  auto serializer = std::unique_ptr<Serializer>{new TextSerializer()};

  auto bodySize = WriteResponse(conn, serializer->Serialize(metrics));

  auto stop_time_of_request = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
      stop_time_of_request - start_time_of_request);
  request_latencies_.Observe(duration.count());

  bytes_transferred_.Increment(bodySize);
  num_scrapes_.Increment();
  return true;
}
*/
std::vector<MetricFamily> MetricsHandler::CollectMetrics() const {
  auto collected_metrics = std::vector<MetricFamily>{};

  for (auto&& wcollectable : collectables_) {
    auto collectable = wcollectable.lock();
    if (!collectable) {
      continue;
    }

    auto&& metrics = collectable->Collect();
    collected_metrics.insert(collected_metrics.end(),
                             std::make_move_iterator(metrics.begin()),
                             std::make_move_iterator(metrics.end()));
  }

  return collected_metrics;
}

/// Proxygen Handler
void MetricsHandler::onRequest(std::unique_ptr<proxygen::HTTPMessage> headers) noexcept {
  if (headers->getMethod().value() != proxygen::HTTPMethod::GET) {
      // Unsupported method
      err_ = 405;
      err_msg_ = "Method Not Allowed";
  }
}

void MetricsHandler::onBody(std::unique_ptr<folly::IOBuf> body) noexcept {

}

void MetricsHandler::onUpgrade(proxygen::UpgradeProtocol prot) noexcept {

}

// TODO(shylock) GZIP support
void MetricsHandler::onEOM() noexcept {
  // Handle error
  if (err_ != 200) {
    proxygen::ResponseBuilder(downstream_)
        .status(err_, err_msg_)
        .sendWithEOM();
    return;
  }


  auto start_time_of_request = std::chrono::steady_clock::now();

  auto metrics = CollectMetrics();

  auto serializer = std::unique_ptr<Serializer>{new TextSerializer()};

  auto body = serializer->Serialize(metrics);

  auto stop_time_of_request = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
      stop_time_of_request - start_time_of_request);
  request_latencies_.Observe(duration.count());

  bytes_transferred_.Increment(body.size());
  num_scrapes_.Increment();

  // Response metrics
  proxygen::ResponseBuilder(downstream_)
      .status(err_, err_msg_)
      .body(body)
      .sendWithEOM();
}

void MetricsHandler::requestComplete() noexcept {
  delete this;
}

void MetricsHandler::onError(proxygen::ProxygenError err) noexcept {
//  LOG(WARNING) << "Get metrics error!";
  delete this;
}

}  // namespace detail
}  // namespace prometheus
