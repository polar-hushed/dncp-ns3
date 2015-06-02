#include "ns3/tag.h"
#include "ns3/uinteger.h"
#include <iostream>

using namespace ns3;

class NodeIdTag : public Tag
{
public:
  static TypeId GetTypeId (void);
  virtual TypeId GetInstanceTypeId (void) const;
  virtual uint32_t GetSerializedSize (void) const;
  virtual void Serialize (TagBuffer i) const;
  virtual void Deserialize (TagBuffer i);
  virtual void Print (std::ostream &os) const;

  // these are our accessors to our tag structure
  void SetNodeId (uint32_t value);
  uint32_t GetNodeId (void) const;
private:
  uint32_t m_nodeid;
};
