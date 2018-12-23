#include "MCP.h"
#include "UCP.h"
#include "Application.h"
#include "ModuleAgentContainer.h"


enum State
{
	ST_INIT,
	ST_REQUESTING_MCCs,
	ST_ITERATING_OVER_MCCs,
	ST_WAITING_ACCEPTANCE,
	ST_NEGOTIATIONS,
	ST_WAITING_UCP,
	ST_NEGOTIATION_FINISHED
};

MCP::MCP(Node *node, uint16_t requestedItemID, uint16_t contributedItemID, unsigned int searchDepth) :
	Agent(node),
	_requestedItemId(requestedItemID),
	_contributedItemId(contributedItemID),
	_searchDepth(searchDepth)
{
	setState(ST_INIT);
}

MCP::~MCP()
{
}

void MCP::update()
{
	switch (state())
	{
	case ST_INIT:
		queryMCCsForItem(_requestedItemId);
		setState(ST_REQUESTING_MCCs);
		break;

	case ST_ITERATING_OVER_MCCs:
		// TODO: Handle this state
		if (_mccRegisterIndex < _mccRegisters.size()) {
			CreateNegotiation(_mccRegisters[_mccRegisterIndex]);
			setState(ST_WAITING_ACCEPTANCE);
		}
		else {
			setState(ST_NEGOTIATION_FINISHED);
			_mccRegisterIndex = 0;
		}
		break;
	case ST_WAITING_ACCEPTANCE:
		break;

	// TODO: Handle other states
	case ST_NEGOTIATIONS:
		if (UCP != nullptr && UCP->negotiationClosed() == true) {
			if (UCP->success == true) { // Negotiation success

				setState(ST_NEGOTIATION_FINISHED);
			}
			else if (UCP->success == false) { // Negotiation failed
				setState(ST_ITERATING_OVER_MCCs);
				_mccRegisterIndex++;
			}
		}
		break;
	case ST_NEGOTIATION_FINISHED:
		DestroyChildUCP();
		break;

	default:;
	}
}

void MCP::stop()
{
	// TODO: Destroy the underlying search hierarchy (UCP->MCP->UCP->...)
	
	DestroyChildUCP();
	destroy();
}

void MCP::OnPacketReceived(TCPSocketPtr socket, const PacketHeader &packetHeader, InputMemoryStream &stream)
{
	const PacketType packetType = packetHeader.packetType;

	switch (packetType)
	{
	case PacketType::ReturnMCCsForItem:
		if (state() == ST_REQUESTING_MCCs)
		{
			// Read the packet
			PacketReturnMCCsForItem packetData;
			packetData.Read(stream);

			// Log the returned MCCs
			for (auto &mccdata : packetData.mccAddresses)
			{
				uint16_t agentId = mccdata.agentId;
				const std::string &hostIp = mccdata.hostIP;
				uint16_t hostPort = mccdata.hostPort;
				//iLog << " - MCC: " << agentId << " - host: " << hostIp << ":" << hostPort;
			}

			// Store the returned MCCs from YP
			_mccRegisters.swap(packetData.mccAddresses);

			// Select the first MCC to negociate
			_mccRegisterIndex = 0;
			setState(ST_ITERATING_OVER_MCCs);

			socket->Disconnect();
		}
		else
		{
			wLog << "OnPacketReceived() - PacketType::ReturnMCCsForItem was unexpected.";
		}
		break;

	// TODO: Handle other packets
	case PacketType::ReturnForNegotiation:
		if (state() == ST_WAITING_ACCEPTANCE) {
			ResponseForNegotiation packetBody;
			packetBody.Read(stream);
			if (packetBody.success == true) {
				iLog << "MCP::Accepted Negotiation";
				CreateChildUCP(packetBody.LocationUCC);
				setState(ST_NEGOTIATIONS);
			}
			else {
				setState(ST_ITERATING_OVER_MCCs);
				_mccRegisterIndex++;
			}
		}
		break;
	default:
		wLog << "OnPacketReceived() - Unexpected PacketType.";
	}
}

bool MCP::negotiationFinished() const
{
	return state() == ST_NEGOTIATION_FINISHED;
}

bool MCP::negotiationAgreement() const
{
	if (UCP != nullptr) {
		return UCP->success == true; // TODO: Did the child UCP find a solution?
	}
	else
		return false; // nope
}


bool MCP::queryMCCsForItem(int itemId)
{
	// Create message header and data
	PacketHeader packetHead;
	packetHead.packetType = PacketType::QueryMCCsForItem;
	packetHead.srcAgentId = id();
	packetHead.dstAgentId = -1;
	PacketQueryMCCsForItem packetData;
	packetData.itemId = _requestedItemId;

	// Serialize message
	OutputMemoryStream stream;
	packetHead.Write(stream);
	packetData.Write(stream);

	// 1) Ask YP for MCC hosting the item 'itemId'
	return sendPacketToYellowPages(stream);
}

void MCP::CreateChildUCP(AgentLocation & LocationUCC)
{
	if (UCP != nullptr)
		DestroyChildUCP();//RESET
	UCP = App->agentContainer->createUCP(node(), requestedItemId(), contributedItemId(), LocationUCC, searchDepth() + 1);//CREATE IT


}

void MCP::DestroyChildUCP()
{
	if (UCP != nullptr) {
		UCP->stop();
		UCP.reset();
	}
}

bool MCP::CreateNegotiation(AgentLocation & LOCATIONMCC)
{
	PacketHeader packethead;
	packethead.packetType = PacketType::RequestForNegotiation;
	packethead.dstAgentId = LOCATIONMCC.agentId;
	packethead.srcAgentId = this->id();

	PacketNegotiationRequest body;
	body._requestedItemId = requestedItemId();
	body._contributedItemId = contributedItemId();

	OutputMemoryStream stream;
	packethead.Write(stream);
	body.Write(stream);

	iLog << "MCP::Asking Negotiation";
	return sendPacketToAgent(LOCATIONMCC.hostIP, LOCATIONMCC.hostPort, stream);

}
