#pragma once
#include "Agent.h"

// Forward declaration
class MCP;
using MCPPtr = std::shared_ptr<MCP>;

class UCP :
	public Agent
{
public:

	// Constructor and destructor
	UCP(Node *node, uint16_t _requestedItemId, uint16_t _contributedItemId, const AgentLocation &uccLoc, unsigned int searchDepth);
	~UCP();

	// Agent methods
	void update() override;
	void stop() override;
	UCP* asUCP() override { return this; }
	void OnPacketReceived(TCPSocketPtr socket, const PacketHeader &packetHeader, InputMemoryStream &stream) override;

	bool success = false;
	// TODO


	uint16_t requestedItemId;
	uint16_t contributedItemId;
	AgentLocation LocationUCC;
	unsigned int searchDepth;

	//new functions
	bool RequestForItem();
	bool ResultConstraint(bool result);
	void createChildMCP(uint16_t newRequestedId);
	void destroyChildMCP();

	MCPPtr MCP;

};

