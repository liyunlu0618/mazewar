#ifndef PACKET_H
#define PACKET_H

#define STATE_UPDATE	0
#define MISSILE_HIT	1
#define MISSILE_HIT_ACK	2
#define LEAVE_GAME	3

#define MAXNAMELEN	16

#include <stdio.h>
#include <string.h>

class PacketHeader {
public:
	uint32_t rat_id;
	uint32_t seq_id;
	uint32_t type;
	char name[MAXNAMELEN];

	PacketHeader() {
	}

	PacketHeader(uint32_t _rat_id, uint32_t _seq_id, uint32_t _type, char* _name) {
		this->rat_id = _rat_id;
		this->seq_id = _seq_id;
		this->type = _type;
		strncpy(this->name, _name, MAXNAMELEN);
	}
};

class StateUpdate : public PacketHeader {
public:
	uint8_t xPos;
	uint8_t yPos;
	uint8_t dir;
	uint8_t cloaked;
	uint32_t score;

	StateUpdate() {
	}

	StateUpdate(uint32_t _rat_id, uint32_t _seq_id, char* _name,
		    uint8_t _xPos, uint8_t _yPos, uint8_t _dir, uint8_t _cloaked, uint32_t _score)
		: PacketHeader(_rat_id, _seq_id, STATE_UPDATE, _name), 
		  xPos(_xPos),
		  yPos(_yPos),
		  dir(_dir),
		  cloaked(_cloaked),
		  score(_score) {}
};

class MissileHit : public PacketHeader {
public:
	uint32_t victimID;

	MissileHit() {
	}

	MissileHit(uint32_t _rat_id, uint32_t _seq_id, char* _name, uint32_t _victimID)
		: PacketHeader(_rat_id, _seq_id, MISSILE_HIT, _name),
		  victimID(_victimID) {}
};

class MissileHitACK : public PacketHeader {
public:
	uint32_t shooterID;

	MissileHitACK() {
	}

	MissileHitACK(uint32_t _rat_id, uint32_t _seq_id, char* _name, uint32_t _shooterID)
		: PacketHeader(_rat_id, _seq_id, MISSILE_HIT_ACK, _name), 
		  shooterID(_shooterID) {}
};

class LeaveGame : public PacketHeader {
public:
	LeaveGame() {
	}

	LeaveGame(uint32_t _rat_id, uint32_t _seq_id, char *_name)
		: PacketHeader(_rat_id, _seq_id, LEAVE_GAME, _name) {}
};

#endif
