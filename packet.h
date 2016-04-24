#ifndef PACKET_H
#define PACKET_H

#define STATE_UPDATE	0
#define MISSILE_HIT	1
#define MISSILE_HIT_ACK	2
#define LEAVE_GAME	3

#define MAXNAMELEN	20

#include <stdio.h>
#include <string.h>

class PacketHeader {
public:
	uint16_t type;
	uint16_t rat_id;
	uint32_t seq_id;
	char name[MAXNAMELEN];

	PacketHeader() {
	}

	PacketHeader(uint16_t _type, uint16_t _rat_id, uint32_t _seq_id, char* _name) {
		this->type = _type;
		this->rat_id = _rat_id;
		this->seq_id = _seq_id;
		strncpy(this->name, _name, MAXNAMELEN);
	}
};

class StateUpdate : public PacketHeader {
public:
	int16_t xPos;
	int16_t yPos;
	int16_t dir;
	int16_t cloaked;
	int32_t score;

	StateUpdate() {
	}

	StateUpdate(uint16_t _rat_id, uint32_t _seq_id, char* _name,
		    int16_t _xPos, int16_t _yPos, int16_t _dir, int16_t _cloaked, int32_t _score)
		: PacketHeader(STATE_UPDATE, _rat_id, _seq_id, _name), 
		  xPos(_xPos),
		  yPos(_yPos),
		  dir(_dir),
		  cloaked(_cloaked),
		  score(_score) {}
};

class MissileHit : public PacketHeader {
public:
	uint16_t victimID;

	MissileHit() {
	}

	MissileHit(uint16_t _rat_id, uint32_t _seq_id, char* _name, uint16_t _victimID)
		: PacketHeader(MISSILE_HIT, _rat_id, _seq_id, _name),
		  victimID(_victimID) {}
};

class MissileHitACK : public PacketHeader {
public:
	uint16_t shooterID;
	int32_t score;

	MissileHitACK() {
	}

	MissileHitACK(uint16_t _rat_id, uint32_t _seq_id, char* _name, uint16_t _shooterID, int32_t _score)
		: PacketHeader(MISSILE_HIT_ACK, _rat_id, _seq_id, _name), 
		  shooterID(_shooterID),
		  score(_score) {}
};

class LeaveGame : public PacketHeader {
public:
	LeaveGame() {
	}

	LeaveGame(uint16_t _rat_id, uint32_t _seq_id, char *_name)
		: PacketHeader(LEAVE_GAME, _rat_id, _seq_id, _name) {}
};

#endif
