#include "UCC.h"


// TODO: Make an enum with the states
enum State
{
	ST_WAITING_REQUEST,
	ST_WAITING_CONSTRAINT,
	ST_NEGOTIATION_CLOSED
};

UCC::UCC(Node *node, uint16_t _contributedItemId, uint16_t _constraintItemId) :
	Agent(node)
{
	// TODO: Save input parameters
	contributedItemId = _contributedItemId;
	constraintItemId = _constraintItemId;

}

UCC::~UCC()
{
}

void UCC::stop()
{
	destroy();
}

void UCC::OnPacketReceived(TCPSocketPtr socket, const PacketHeader &packetHeader, InputMemoryStream &stream)
{
	PacketType packetType = packetHeader.packetType;

	switch (packetType)
	{
	case PacketType::RequestForItem:
		if (state() == ST_WAITING_REQUEST) {
			RequestForItem packetBody;
			packetBody.Read(stream);
			// Sending ConstraintRequest to UCP
			PacketHeader oPacketHeader;
			oPacketHeader.srcAgentId = id();
			oPacketHeader.dstAgentId = packetHeader.srcAgentId;
			oPacketHeader.packetType = PacketType::RequestForConstraint;
			OutputMemoryStream ostream;
			oPacketHeader.Write(ostream);
			RequestForConstraint oPacketBody;
			oPacketBody.Id = constraintItemId;
			oPacketBody.Write(ostream);
			iLog << "UCC::Sending ConstraintRequest";
			socket->SendPacket(ostream.GetBufferPtr(), ostream.GetSize());

			setState(ST_WAITING_CONSTRAINT);
		}
		else {
			wLog << "UCC::PacketReceived() - Unexpected Item Request";
		}
		break;

	case PacketType::ResultForConstraint:
		if (state() == ST_WAITING_CONSTRAINT) {
			RequestForResult packetBody;
			packetBody.Read(stream);
			if (packetBody.success == true) {
				negociation_success = true;
			}
			else {
				negociation_success = false;
			}
			// Sending ConstraintAck to UCP
			PacketHeader oPacketHeader;
			oPacketHeader.packetType = PacketType::AcknowledgeForConstraint;
			oPacketHeader.srcAgentId = id();
			oPacketHeader.dstAgentId = packetHeader.srcAgentId;
			OutputMemoryStream ostream;
			oPacketHeader.Write(ostream);
			iLog << "UCC::Sending ConstraintAck";
			socket->SendPacket(ostream.GetBufferPtr(), ostream.GetSize());

			setState(ST_NEGOTIATION_CLOSED);
		}
		else {
			wLog << "UCC::PacketReceived() - Unexpected Constraint Result";
		}
		break;
	default:
		wLog << "OnPacketReceived() - Unexpected PacketType.";
	}
}

bool UCC::NegotiationSuccess()
{
	bool ret = false;

	if (state() == ST_NEGOTIATION_CLOSED && negociation_success)
		ret = true;

	return ret;
}

bool UCC::NegotiationClosed()
{
	bool ret = false;

	ret = state() == ST_NEGOTIATION_CLOSED;

	return ret;
}
