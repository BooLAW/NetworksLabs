#include "UCP.h"
#include "MCP.h"
#include "Application.h"
#include "ModuleAgentContainer.h"


// TODO: Make an enum with the states
enum State {
	ST_INIT,
	ST_ITEM_REQUEST,
	ST_CONSTRAINT_CALCULATING,
	ST_CONSTRAINT_SENT,
	ST_NEGOTIATION_CLOSED

};

UCP::UCP(Node *node, uint16_t _requestedItemId, uint16_t _contributedItemId, const AgentLocation &uccLocation, unsigned int _searchDepth) :
	Agent(node)
{
	// TODO: Save input parameters
	requestedItemId = _requestedItemId;
	contributedItemId = _contributedItemId;
	LocationUCC = uccLocation;
	searchDepth = _searchDepth;
}

UCP::~UCP()
{
}

void UCP::update()
{
	switch (state())
	{
		// TODO: Handle states
	case ST_INIT:
		success = -1;
		RequestForItem();
		setState(ST_ITEM_REQUEST);
		break;

	case ST_CONSTRAINT_CALCULATING:
		if (MCP->negotiationFinished()) {
			if (MCP->negotiationAgreement()) {
				ResultConstraint(true);
				success = true;

			}
			else {
				ResultConstraint(false);
				success = false;
			}
			setState(ST_CONSTRAINT_SENT);
		}
		break;
	default:;
	}
}

void UCP::stop()
{
	// TODO: Destroy search hierarchy below this agent
	DestroyChildMCP();
	destroy();
}

void UCP::OnPacketReceived(TCPSocketPtr socket, const PacketHeader &packetHeader, InputMemoryStream &stream)
{
	PacketType packetType = packetHeader.packetType;

	switch (packetType)
	{
	case PacketType::RequestForConstraint:
		RequestForConstraint packetbody;
		packetbody.Read(stream);
		if (packetbody.Id == this->contributedItemId) {
			success = true;
			ResultConstraint(success);
			setState(ST_CONSTRAINT_SENT);
		}
		else
		{
			if (searchDepth >= MAX_SEARCH_DEPTH) {
				success = false;
				ResultConstraint(success);
				wLog << "Max Depth Reached";
				setState(ST_CONSTRAINT_SENT);
			}
			else {
				iLog << "UCP::Constraint Unresolved";
				createChildMCP(packetbody.Id);
				setState(ST_CONSTRAINT_CALCULATING);
			}
		}
		break;

	case PacketType::AcknowledgeForConstraint:
		if (state() == ST_CONSTRAINT_SENT) {
			setState(ST_NEGOTIATION_CLOSED);
			iLog << "UCP::Constraint aknowledged";
		}
		else {
			iLog << "UCP::OnPacketReceived() - Unexpected ConstraintAck";
		}
		break;

	default:
		wLog << "OnPacketReceived() - Unexpected PacketType.";
	}
}

bool UCP::RequestForItem()
{
	PacketHeader packethead;
	packethead.packetType = PacketType::RequestForItem;
	packethead.dstAgentId = LocationUCC.agentId;
	packethead.srcAgentId = this->id();

	RequestItem body;
	body.Id = this->requestedItemId;
	OutputMemoryStream stream;
	packethead.Write(stream);
	body.Write(stream);

	iLog << "UCP::Sending ItemRequest";

	return sendPacketToAgent(LocationUCC.hostIP, LocationUCC.hostPort, stream);
}

bool UCP::ResultConstraint(bool result)
{
	PacketHeader packethead;
	packethead.packetType = PacketType::ResultForConstraint;
	packethead.dstAgentId = LocationUCC.agentId;
	packethead.srcAgentId = this->id();

	RequestForResult body;
	body.success = result;
	OutputMemoryStream stream;
	packethead.Write(stream);
	body.Write(stream);

	iLog << "UCP::Sending Constraint Result:";
	iLog << body.success;

	return sendPacketToAgent(LocationUCC.hostIP, LocationUCC.hostPort, stream);
}

void UCP::createChildMCP(uint16_t newRequestedId)
{
	if (MCP != nullptr)
		DestroyChildMCP();//reset it
	else
		MCP = App->agentContainer->createMCP(node(), newRequestedId, contributedItemId, searchDepth);//create it

}

void UCP::DestroyChildMCP()
{
	if (MCP != nullptr) {
		MCP->stop();
		MCP.reset();
	}
}

bool UCP::negotiationClosed()
{
	return state() == ST_NEGOTIATION_CLOSED;
}
