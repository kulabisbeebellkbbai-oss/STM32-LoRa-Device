#pragma once

#include <cstdint>
#include <cstring>

namespace lora_message {

constexpr uint16_t kNodeIdentityRegionInvalid = 0;
constexpr uint32_t kNodeIdentityUnknown = 0xFFFFFFFFU;

struct NodeIdentity {
  uint16_t regionId{0};
  uint32_t nodeId{0};
  uint16_t category{0};
};

struct NodeContact {
  NodeIdentity identity{};
  char displayName[24]{};
  char callsign[24]{};
  char contactEmail[40]{};
};

struct IdentityMatch {
  bool valid{false};
  bool sameNode{false};
  bool sameRegion{false};
};

inline bool operator==(const NodeIdentity &lhs, const NodeIdentity &rhs) {
  return lhs.regionId == rhs.regionId && lhs.nodeId == rhs.nodeId && lhs.category == rhs.category;
}

inline bool operator!=(const NodeIdentity &lhs, const NodeIdentity &rhs) {
  return !(lhs == rhs);
}

bool isValidIdentity(const NodeIdentity &identity);
bool isIdentityZero(const NodeIdentity &identity);
IdentityMatch compareIdentity(const NodeIdentity &left, const NodeIdentity &right);
void clearIdentity(NodeIdentity &identity);
void clearContact(NodeContact &contact);

bool setContactDisplayName(NodeContact &contact, const char *value);
bool setContactCallsign(NodeContact &contact, const char *value);
bool setContactEmail(NodeContact &contact, const char *value);

}  // namespace lora_message
