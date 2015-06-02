#include "node-id-tag.h"
#include "ns3/packet.h"

using namespace ns3;



TypeId
NodeIdTag::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::NodeIdTag")
    .SetParent<Tag> ()
    .AddConstructor<NodeIdTag> ()
    .AddAttribute ("NodeId",
                   "ID of the node",
                   EmptyAttributeValue (),
                   MakeUintegerAccessor (&NodeIdTag::GetNodeId),
                   MakeUintegerChecker<uint32_t> ())
  ;
  return tid;
}
TypeId
NodeIdTag::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}
uint32_t
NodeIdTag::GetSerializedSize (void) const
{
  return 1;
}
void
NodeIdTag::Serialize (TagBuffer i) const
{
  i.WriteU8 (m_nodeid);
}
void
NodeIdTag::Deserialize (TagBuffer i)
{
  m_nodeid= i.ReadU8 ();
}
void
NodeIdTag::Print (std::ostream &os) const
{
  os << "node ID=" << m_nodeid;
}
void
NodeIdTag::SetNodeId (uint32_t value)
{
  m_nodeid = value;
}
uint32_t
NodeIdTag::GetNodeId (void) const
{
  return m_nodeid;
}

