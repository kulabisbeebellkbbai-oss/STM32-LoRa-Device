#include "node_identity.h"

#include <cstddef>
#include <cstring>

namespace {
constexpr size_t kNameMax = sizeof(lora_message::NodeContact::displayName) - 1;
constexpr size_t kCallsignMax = sizeof(lora_message::NodeContact::callsign) - 1;
constexpr size_t kEmailMax = sizeof(lora_message::NodeContact::contactEmail) - 1;
}  // namespace

namespace lora_message {

bool isValidIdentity(const NodeIdentity &identity) {
  return identity.regionId != kNodeIdentityRegionInvalid && identity.nodeId != kNodeIdentityUnknown;
}

bool isIdentityZero(const NodeIdentity &identity) {
  return identity.regionId == 0 && identity.nodeId == 0 && identity.category == 0;
}

IdentityMatch compareIdentity(const NodeIdentity &left, const NodeIdentity &right) {
  return {left == right, left.regionId == right.regionId && left.nodeId == right.nodeId,
          left.regionId == right.regionId};
}

void clearIdentity(NodeIdentity &identity) {
  identity = NodeIdentity{};
}

void clearContact(NodeContact &contact) {
  contact = NodeContact{};
}

bool setContactDisplayName(NodeContact &contact, const char *value) {
  if (value == nullptr) {
    contact.displayName[0] = '\0';
    return true;
  }
  const size_t len = std::strlen(value);
  const size_t clipped = (len > kNameMax) ? kNameMax : len;
  std::memcpy(contact.displayName, value, clipped);
  contact.displayName[clipped] = '\0';
  return len <= kNameMax;
}

bool setContactCallsign(NodeContact &contact, const char *value) {
  if (value == nullptr) {
    contact.callsign[0] = '\0';
    return true;
  }
  const size_t len = std::strlen(value);
  const size_t clipped = (len > kCallsignMax) ? kCallsignMax : len;
  std::memcpy(contact.callsign, value, clipped);
  contact.callsign[clipped] = '\0';
  return len <= kCallsignMax;
}

bool setContactEmail(NodeContact &contact, const char *value) {
  if (value == nullptr) {
    contact.contactEmail[0] = '\0';
    return true;
  }
  const size_t len = std::strlen(value);
  const size_t clipped = (len > kEmailMax) ? kEmailMax : len;
  std::memcpy(contact.contactEmail, value, clipped);
  contact.contactEmail[clipped] = '\0';
  return len <= kEmailMax;
}

}  // namespace lora_message
