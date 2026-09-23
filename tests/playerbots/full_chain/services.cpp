#include "services.h"
#include <stdexcept>
AuditConfig sPlayerbotAIConfig;
AuditMonitor sPerformanceMonitor;
Unit auditUnit;
PerformanceMonitorOperation::~PerformanceMonitorOperation() = default;
std::unique_ptr<PerformanceMonitorOperation> AuditMonitor::start(int,const std::string&,PlayerbotAI*) {
    throw std::runtime_error("PMO must remain off for this audit");
}
