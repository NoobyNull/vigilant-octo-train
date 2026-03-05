#include "project_costing_io.h"

#include <chrono>
#include <ctime>

#include <nlohmann/json.hpp>

#include "../utils/file_utils.h"
#include "../utils/log.h"

namespace dw {

namespace {

static const char* kLogModule = "CostingIO";

std::string currentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S", std::localtime(&time));
    return buf;
}

nlohmann::json snapshotToJson(const CostingSnapshot& snap) {
    return {{"db_id", snap.dbId},
            {"name", snap.name},
            {"dimensions", snap.dimensions},
            {"price_at_capture", snap.priceAtCapture}};
}

CostingSnapshot jsonToSnapshot(const nlohmann::json& j) {
    CostingSnapshot s;
    s.dbId = j.value("db_id", static_cast<i64>(0));
    s.name = j.value("name", std::string{});
    s.dimensions = j.value("dimensions", std::string{});
    s.priceAtCapture = j.value("price_at_capture", 0.0);
    return s;
}

nlohmann::json entryToJson(const CostingEntry& e) {
    return {{"id", e.id},
            {"name", e.name},
            {"category", e.category},
            {"quantity", e.quantity},
            {"unit_rate", e.unitRate},
            {"estimated_total", e.estimatedTotal},
            {"actual_total", e.actualTotal},
            {"unit", e.unit},
            {"notes", e.notes},
            {"snapshot", snapshotToJson(e.snapshot)}};
}

CostingEntry jsonToEntry(const nlohmann::json& j) {
    CostingEntry e;
    e.id = j.value("id", std::string{});
    e.name = j.value("name", std::string{});
    e.category = j.value("category", std::string{});
    e.quantity = j.value("quantity", 0.0);
    e.unitRate = j.value("unit_rate", 0.0);
    e.estimatedTotal = j.value("estimated_total", 0.0);
    e.actualTotal = j.value("actual_total", 0.0);
    e.unit = j.value("unit", std::string{});
    e.notes = j.value("notes", std::string{});
    if (j.contains("snapshot")) {
        e.snapshot = jsonToSnapshot(j["snapshot"]);
    }
    return e;
}

nlohmann::json estimateToJson(const CostingEstimate& est) {
    nlohmann::json j;
    j["name"] = est.name;
    j["sale_price"] = est.salePrice;
    j["notes"] = est.notes;
    j["created_at"] = est.createdAt;
    j["modified_at"] = est.modifiedAt;
    j["entries"] = nlohmann::json::array();
    for (const auto& e : est.entries) {
        j["entries"].push_back(entryToJson(e));
    }
    return j;
}

CostingEstimate jsonToEstimate(const nlohmann::json& j) {
    CostingEstimate est;
    est.name = j.value("name", std::string{});
    est.salePrice = j.value("sale_price", 0.0);
    est.notes = j.value("notes", std::string{});
    est.createdAt = j.value("created_at", std::string{});
    est.modifiedAt = j.value("modified_at", std::string{});
    if (j.contains("entries")) {
        for (const auto& e : j["entries"]) {
            est.entries.push_back(jsonToEntry(e));
        }
    }
    return est;
}

nlohmann::json orderToJson(const CostingOrder& order) {
    nlohmann::json j;
    j["name"] = order.name;
    j["sale_price"] = order.salePrice;
    j["notes"] = order.notes;
    j["created_at"] = order.createdAt;
    j["finalized_at"] = order.finalizedAt;
    j["source_estimate_name"] = order.sourceEstimateName;
    j["entries"] = nlohmann::json::array();
    for (const auto& e : order.entries) {
        j["entries"].push_back(entryToJson(e));
    }
    return j;
}

CostingOrder jsonToOrder(const nlohmann::json& j) {
    CostingOrder order;
    order.name = j.value("name", std::string{});
    order.salePrice = j.value("sale_price", 0.0);
    order.notes = j.value("notes", std::string{});
    order.createdAt = j.value("created_at", std::string{});
    order.finalizedAt = j.value("finalized_at", std::string{});
    order.sourceEstimateName = j.value("source_estimate_name", std::string{});
    if (j.contains("entries")) {
        for (const auto& e : j["entries"]) {
            order.entries.push_back(jsonToEntry(e));
        }
    }
    return order;
}

} // namespace

// --- saveEstimates ---

bool ProjectCostingIO::saveEstimates(const Path& costingDir,
                                     const std::vector<CostingEstimate>& estimates) {
    file::createDirectories(costingDir);

    nlohmann::json j;
    j["schema_version"] = SCHEMA_VERSION;
    j["estimates"] = nlohmann::json::array();
    for (const auto& est : estimates) {
        j["estimates"].push_back(estimateToJson(est));
    }

    Path filePath = Path(costingDir) / "estimates.json";
    if (!file::writeText(filePath, j.dump(2))) {
        log::errorf(kLogModule, "Failed to write estimates: %s", filePath.c_str());
        return false;
    }
    return true;
}

// --- loadEstimates ---

std::vector<CostingEstimate> ProjectCostingIO::loadEstimates(const Path& costingDir) {
    Path filePath = Path(costingDir) / "estimates.json";
    auto text = file::readText(filePath);
    if (!text) {
        return {};
    }

    try {
        auto j = nlohmann::json::parse(*text);
        // Check schema version for future compatibility
        (void)j.value("schema_version", 1);

        std::vector<CostingEstimate> estimates;
        if (j.contains("estimates")) {
            for (const auto& e : j["estimates"]) {
                estimates.push_back(jsonToEstimate(e));
            }
        }
        return estimates;
    } catch (const nlohmann::json::exception& ex) {
        log::errorf(kLogModule, "JSON parse error: %s", ex.what());
        return {};
    }
}

// --- saveOrders ---

bool ProjectCostingIO::saveOrders(const Path& costingDir,
                                  const std::vector<CostingOrder>& orders) {
    file::createDirectories(costingDir);

    nlohmann::json j;
    j["schema_version"] = SCHEMA_VERSION;
    j["orders"] = nlohmann::json::array();
    for (const auto& order : orders) {
        j["orders"].push_back(orderToJson(order));
    }

    Path filePath = Path(costingDir) / "orders.json";
    if (!file::writeText(filePath, j.dump(2))) {
        log::errorf(kLogModule, "Failed to write orders: %s", filePath.c_str());
        return false;
    }
    return true;
}

// --- loadOrders ---

std::vector<CostingOrder> ProjectCostingIO::loadOrders(const Path& costingDir) {
    Path filePath = Path(costingDir) / "orders.json";
    auto text = file::readText(filePath);
    if (!text) {
        return {};
    }

    try {
        auto j = nlohmann::json::parse(*text);
        (void)j.value("schema_version", 1);

        std::vector<CostingOrder> orders;
        if (j.contains("orders")) {
            for (const auto& o : j["orders"]) {
                orders.push_back(jsonToOrder(o));
            }
        }
        return orders;
    } catch (const nlohmann::json::exception& ex) {
        log::errorf(kLogModule, "JSON parse error: %s", ex.what());
        return {};
    }
}

// --- convertToOrder ---

CostingOrder ProjectCostingIO::convertToOrder(const CostingEstimate& estimate) {
    CostingOrder order;
    order.name = estimate.name;
    order.entries = estimate.entries; // Deep copy
    order.salePrice = estimate.salePrice;
    order.notes = estimate.notes;
    order.createdAt = estimate.createdAt;
    order.finalizedAt = currentTimestamp();
    order.sourceEstimateName = estimate.name;
    return order;
}

} // namespace dw
