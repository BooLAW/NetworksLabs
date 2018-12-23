#include "MCC.h"
#include "UCC.h"
#include "Application.h"
#include "ModuleAgentContainer.h"


enum State
{
	ST_INIT,
	ST_REGISTERING,
	ST_IDLE,
	ST_NEGOTIATIONS,
	ST_UNREGISTERING,
	ST_FINISHED
};

MCC::MCC(Node *node, uint16_t contributedItemId, uint16_t constraintItemId) :
	Agent(node),
	_contributedItemId(contributedItemId),
	_constraintItemId(constraintItemId)
{
	setState(ST_INIT);
}


MCC::~MCC()
{
}

void MCC::update()
{
	switch (state())
	{
	case ST_INIT:
		if (registerIntoYellowPages()) {
			setState(ST_REGISTERING);
		}
		else {
			setState(ST_FINISHED);
		}
		break;

	case ST_REGISTERING:
		// See OnPacketReceived()
		break;

		// TODO: Handle other states
	case ST_NEGOTIATIONS:
		if (UCC != nullptr && UCC->NegotiationClosed() == true) {
			if (negotiationAgreement()) {
				setState(ST_FINISHED);																	////// SISMISMEISIAIE
			}
			destroyChildUCC();
		}
		break;
	case ST_FINISHED:
		destroy();
	}
}

void MCC::stop()
{
	// Destroy hierarchy below this agent (only a UCC, actually)
	destroyChildUCC();

	unregisterFromYellowPages();
	setState(ST_FINISHED);
}


void MCC::OnPacketReceived(TCPSocketPtr socket, const PacketHeader &packetHeader, InputMemoryStream &stream)
{
	const PacketType packetType = packetHeader.packetType;

	switch (packetType)
	{
	case PacketType::RegisterMCCAck:
		if (state() == ST_REGISTERING)
		{
			setState(ST_IDLE);
			socket->Disconnect();
		}
		else
		{
			wLog << "OnPacketReceived() - PacketType::RegisterMCCAck was unexpected.";
		}
		break;

	// TODO: Handle other packets
		case PacketType::RequestForNegotiation:
		if (state() == ST_IDLE) 
		{
			AgentLocation uccLoc;
			createChildUCC();
			uccLoc.agentId = UCC->id();
			uccLoc.hostIP = socket->RemoteAddress().GetIPString();
			uccLoc.hostPort = LISTEN_PORT_AGENTS;
			acceptNegotiation(socket, packetHeader.srcAgentId, true, uccLoc);
			setState(ST_NEGOTIATIONS);
		}
		else
		{
			AgentLocation uccLoc;
			acceptNegotiation(socket, packetHeader.srcAgentId, false, uccLoc);
			wLog << "MCC::OnPacketReceived() - PacketType::NegotiationRequest was unexpected.";
		}
		break;
	default:
		wLog << "OnPacketReceived() - Unexpected PacketType.";
	}
}

bool MCC::isIdling() const
{
	return state() == ST_IDLE;
}

bool MCC::negotiationFinished() const
{
	return state() == ST_FINISHED;
}

bool MCC::negotiationAgreement() const
{
	// If this agent finished, means that it was an agreement
	// Otherwise, it would return to state ST_IDLE
	return negotiationFinished();
}

bool MCC::acceptNegotiation(TCPSocketPtr socket, uint16_t dstID, bool accept, AgentLocation &uccLoc)
{
	PacketHeader packetHead;
	packetHead.packetType = PacketType::ReturnForNegotiation;
	packetHead.srcAgentId = id();
	packetHead.dstAgentId = dstID;

	ResponseForNegotiation packetBody;
	packetBody.success = accept;
	packetBody.LocationUCC = uccLoc;

	// Serialize
	OutputMemoryStream stream;
	packetHead.Write(stream);
	packetBody.Write(stream);

	iLog << "MCC::Sending Negotiation Response";
	iLog << accept;

	socket->SendPacket(stream.GetBufferPtr(), stream.GetSize());

	return false;
}

bool MCC::registerIntoYellowPages()
{
	// Create message header and data
	PacketHeader packetHead;
	packetHead.packetType = PacketType::RegisterMCC;
	packetHead.srcAgentId = id();
	packetHead.dstAgentId = -1;
	PacketRegisterMCC packetData;
	packetData.itemId = _contributedItemId;

	// Serialize message
	OutputMemoryStream stream;
	packetHead.Write(stream);
	packetData.Write(stream);

	return sendPacketToYellowPages(stream);
}

void MCC::unregisterFromYellowPages()
{
	// Create message
	PacketHeader packetHead;
	packetHead.packetType = PacketType::UnregisterMCC;
	packetHead.srcAgentId = id();
	packetHead.dstAgentId = -1;
	PacketUnregisterMCC packetData;
	packetData.itemId = _contributedItemId;

	// Serialize message
	OutputMemoryStream stream;
	packetHead.Write(stream);
	packetData.Write(stream);

	sendPacketToYellowPages(stream);
}

void MCC::createChildUCC()
{
	// TODO: Create a unicast contributor
	if (UCC != nullptr)
		destroyChildUCC();
	iLog << "UCC Child created";
	UCC = App->agentContainer->createUCC(node(), contributedItemId(), constraintItemId());

}

void MCC::destroyChildUCC()
{
	// TODO: Destroy the unicast contributor child
	if (UCC != nullptr){
		iLog << "UCC Child destroyed ";
		UCC->stop();
		UCC.reset();
	}
}
