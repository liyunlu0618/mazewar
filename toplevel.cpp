/*
 *   FILE: toplevel.c
 * AUTHOR: name (email)
 *   DATE: March 31 23:59:59 PST 2013
 *  DESCR:
 */

/* #define DEBUG */

#include "main.h"
#include <string>
#include "mazewar.h"
#include <assert.h>

static bool		updateView;	/* true if update needed */
MazewarInstance::Ptr M;

/* Use this socket address to send packets to the multi-cast group. */
static Sockaddr         groupAddr;
#define MAX_OTHER_RATS  (MAX_RATS - 1)
static int seq[PACKET_TYPE];

#define HEARTBEAT_SPEED	1000

int main(int argc, char *argv[])
{
    Loc x(1);
    Loc y(5);
    Direction dir(0);
    char *ratName;

    signal(SIGHUP, quit);
    signal(SIGINT, quit);
    signal(SIGTERM, quit);

    getName("Welcome to CS244B MazeWar!\n\nYour Name", &ratName);
    ratName[strlen(ratName)-1] = 0;

    M = MazewarInstance::mazewarInstanceNew(string(ratName));
    MazewarInstance* a = M.ptr();
    strncpy(M->myName_, ratName, NAMESIZE);
    free(ratName);

    MazeInit(argc, argv);

    NewPosition(M);

    joinGame();

    /* So you can see what a Rat is supposed to look like, we create
    one rat in the single player mode Mazewar.
    It doesn't move, you can't shoot it, you can just walk around it */

    play();

    return 0;
}


/* ----------------------------------------------------------------------- */

void
play(void)
{
	MWEvent		event;
	MW244BPacket	incoming;

	event.eventDetail = &incoming;

	while (TRUE) {
		NextEvent(&event, M->theSocket());
		if (!M->peeking())
			switch(event.eventType) {
			case EVENT_A:
				aboutFace();
				sendStateUpdate();
				break;

			case EVENT_S:
				leftTurn();
				sendStateUpdate();
				break;

			case EVENT_D:
				forward();
				sendStateUpdate();
				break;

			case EVENT_F:
				rightTurn();
				sendStateUpdate();
				break;

                        case EVENT_G:
				backward();
				sendStateUpdate();
				break;

                        case EVENT_C:
				cloak();
				sendStateUpdate();
				break;

			case EVENT_BAR:
				shoot();
				break;

			case EVENT_LEFT_D:
				peekLeft();
				break;


			case EVENT_RIGHT_D:
				peekRight();
				break;

			case EVENT_NETWORK:
				processPacket(&event);
				break;

			case EVENT_INT:
				sendLeaveGame();
				quit(0);
				break;

			default:
				break;

			}
		else
			switch (event.eventType) {
			case EVENT_RIGHT_U:
			case EVENT_LEFT_U:
				peekStop();
				break;

			case EVENT_NETWORK:
				processPacket(&event);
				break;

			default:
				break;
			}

		ratStates();		/* clean house */

		manageMissiles();

		DoViewUpdate();

		sendHeartBeat();

		/* Any info to send over network? */

	}
}

/* ----------------------------------------------------------------------- */

static	Direction	_aboutFace[NDIRECTION] ={SOUTH, NORTH, WEST, EAST};
static	Direction	_leftTurn[NDIRECTION] =	{WEST, EAST, NORTH, SOUTH};
static	Direction	_rightTurn[NDIRECTION] ={EAST, WEST, SOUTH, NORTH};

void
aboutFace(void)
{
	M->dirIs(_aboutFace[MY_DIR]);
	updateView = TRUE;
}

/* ----------------------------------------------------------------------- */

void
leftTurn(void)
{
	M->dirIs(_leftTurn[MY_DIR]);
	updateView = TRUE;
}

/* ----------------------------------------------------------------------- */

void
rightTurn(void)
{
	M->dirIs(_rightTurn[MY_DIR]);
	updateView = TRUE;
}

/* ----------------------------------------------------------------------- */

/* remember ... "North" is to the right ... positive X motion */

void
forward(void)
{
	register int	tx = MY_X_LOC;
	register int	ty = MY_Y_LOC;

	switch(MY_DIR) {
	case NORTH:	if (!M->maze_[tx+1][ty])	tx++; break;
	case SOUTH:	if (!M->maze_[tx-1][ty])	tx--; break;
	case EAST:	if (!M->maze_[tx][ty+1])	ty++; break;
	case WEST:	if (!M->maze_[tx][ty-1])	ty--; break;
	default:
		MWError("bad direction in Forward");
	}
	if ((MY_X_LOC != tx) || (MY_Y_LOC != ty)) {
		M->xlocIs(Loc(tx));
		M->ylocIs(Loc(ty));
		updateView = TRUE;
	}
}

/* ----------------------------------------------------------------------- */

void backward()
{
	register int	tx = MY_X_LOC;
	register int	ty = MY_Y_LOC;

	switch(MY_DIR) {
	case NORTH:	if (!M->maze_[tx-1][ty])	tx--; break;
	case SOUTH:	if (!M->maze_[tx+1][ty])	tx++; break;
	case EAST:	if (!M->maze_[tx][ty-1])	ty--; break;
	case WEST:	if (!M->maze_[tx][ty+1])	ty++; break;
	default:
		MWError("bad direction in Backward");
	}
	if ((MY_X_LOC != tx) || (MY_Y_LOC != ty)) {
		M->xlocIs(Loc(tx));
		M->ylocIs(Loc(ty));
		updateView = TRUE;
	}
}

/* ----------------------------------------------------------------------- */

void peekLeft()
{
	M->xPeekIs(MY_X_LOC);
	M->yPeekIs(MY_Y_LOC);
	M->dirPeekIs(MY_DIR);

	switch(MY_DIR) {
	case NORTH:	if (!M->maze_[MY_X_LOC+1][MY_Y_LOC]) {
				M->xPeekIs(MY_X_LOC + 1);
				M->dirPeekIs(WEST);
			}
			break;

	case SOUTH:	if (!M->maze_[MY_X_LOC-1][MY_Y_LOC]) {
				M->xPeekIs(MY_X_LOC - 1);
				M->dirPeekIs(EAST);
			}
			break;

	case EAST:	if (!M->maze_[MY_X_LOC][MY_Y_LOC+1]) {
				M->yPeekIs(MY_Y_LOC + 1);
				M->dirPeekIs(NORTH);
			}
			break;

	case WEST:	if (!M->maze_[MY_X_LOC][MY_Y_LOC-1]) {
				M->yPeekIs(MY_Y_LOC - 1);
				M->dirPeekIs(SOUTH);
			}
			break;

	default:
			MWError("bad direction in PeekLeft");
	}

	/* if any change, display the new view without moving! */

	if ((M->xPeek() != MY_X_LOC) || (M->yPeek() != MY_Y_LOC)) {
		M->peekingIs(TRUE);
		updateView = TRUE;
	}
}

/* ----------------------------------------------------------------------- */

void peekRight()
{
	M->xPeekIs(MY_X_LOC);
	M->yPeekIs(MY_Y_LOC);
	M->dirPeekIs(MY_DIR);

	switch(MY_DIR) {
	case NORTH:	if (!M->maze_[MY_X_LOC+1][MY_Y_LOC]) {
				M->xPeekIs(MY_X_LOC + 1);
				M->dirPeekIs(EAST);
			}
			break;

	case SOUTH:	if (!M->maze_[MY_X_LOC-1][MY_Y_LOC]) {
				M->xPeekIs(MY_X_LOC - 1);
				M->dirPeekIs(WEST);
			}
			break;

	case EAST:	if (!M->maze_[MY_X_LOC][MY_Y_LOC+1]) {
				M->yPeekIs(MY_Y_LOC + 1);
				M->dirPeekIs(SOUTH);
			}
			break;

	case WEST:	if (!M->maze_[MY_X_LOC][MY_Y_LOC-1]) {
				M->yPeekIs(MY_Y_LOC - 1);
				M->dirPeekIs(NORTH);
			}
			break;

	default:
			MWError("bad direction in PeekRight");
	}

	/* if any change, display the new view without moving! */

	if ((M->xPeek() != MY_X_LOC) || (M->yPeek() != MY_Y_LOC)) {
		M->peekingIs(TRUE);
		updateView = TRUE;
	}
}

/* ----------------------------------------------------------------------- */

void peekStop()
{
	M->peekingIs(FALSE);
	updateView = TRUE;
}

/* ----------------------------------------------------------------------- */

void shoot()
{
	if (M->hasMissile()) return;

	M->scoreIs( M->score().value()-1 );
	UpdateScoreCard(MY_RAT_INDEX);

	M->hasMissileIs(TRUE);
	M->xMissileIs(M->xloc());
	M->yMissileIs(M->yloc());
	M->dirMissileIs(M->dir());

	timeval t;
	gettimeofday(&t, NULL);
	M->lastUpdateIs(t);

	updateView = TRUE;
}

/* ----------------------------------------------------------------------- */

void cloak()
{
        printf("Implement cloak()\n");
}

/* ----------------------------------------------------------------------- */

/*
 * Exit from game, clean up window
 */

void quit(int sig)
{

	StopWindow();
	exit(0);
}


/* ----------------------------------------------------------------------- */

void NewPosition(MazewarInstance::Ptr m)
{
	Loc newX(0);
	Loc newY(0);
	Direction dir(0); /* start on occupied square */

	while (M->maze_[newX.value()][newY.value()]) {
	  /* MAZE[XY]MAX is a power of 2 */
	  newX = Loc(random() & (MAZEXMAX - 1));
	  newY = Loc(random() & (MAZEYMAX - 1));

	  /* In real game, also check that square is
	     unoccupied by another rat */
	}

	/* prevent a blank wall at first glimpse */

	if (!m->maze_[(newX.value())+1][(newY.value())]) dir = Direction(NORTH);
	if (!m->maze_[(newX.value())-1][(newY.value())]) dir = Direction(SOUTH);
	if (!m->maze_[(newX.value())][(newY.value())+1]) dir = Direction(EAST);
	if (!m->maze_[(newX.value())][(newY.value())-1]) dir = Direction(WEST);

	m->xlocIs(newX);
	m->ylocIs(newY);
	m->dirIs(dir);
}

/* ----------------------------------------------------------------------- */

void MWError(char *s)

{
	StopWindow();
	fprintf(stderr, "CS244BMazeWar: %s\n", s);
	perror("CS244BMazeWar");
	exit(-1);
}

/* ----------------------------------------------------------------------- */

/* This is just for the sample version, rewrite your own */
Score GetRatScore(RatIndexType ratId)
{
	if (ratId.value() == MY_RAT_INDEX) {
		return M->score();
	} else {
		return M->rat(ratId).score;
	}
}

/* ----------------------------------------------------------------------- */

/* This is just for the sample version, rewrite your own */
char *GetRatName(RatIndexType ratId)
{
	if (ratId.value() == MY_RAT_INDEX) {
		return M->myName_;
	} else {
		return M->rat(ratId).name;
	}
}

/* ----------------------------------------------------------------------- */

/* This is just for the sample version, rewrite your own if necessary */
void ConvertIncoming(MW244BPacket *p)
{
}

/* ----------------------------------------------------------------------- */

/* This is just for the sample version, rewrite your own if necessary */
void ConvertOutgoing(MW244BPacket *p)
{
}

/* ----------------------------------------------------------------------- */

/* This is just for the sample version, rewrite your own */
void ratStates()
{
  /* In our sample version, we don't know about the state of any rats over
     the net, so this is a no-op */
}

/* ----------------------------------------------------------------------- */

/* This is just for the sample version, rewrite your own */
void manageMissiles()
{
	if (!M->hasMissile()) return;

	timeval last, now;
	gettimeofday(&now, NULL);
	last = M->lastUpdate();
	if ((now.tv_sec - last.tv_sec) * 1000
		+ (now.tv_usec - last.tv_usec) / 1000 < MISSILE_SPEED) return;

	M->lastUpdateIs(now);
	int oldX = M->xMissile().value();
	int oldY = M->yMissile().value();
	int newX = oldX;
	int newY = oldY;
	switch (MY_DIR) {
		case NORTH:
	  		newX = oldX + 1;
			break;
		case SOUTH:
			newX = oldX - 1;
			break;
		case EAST:
			newY = oldY + 1;
			break;
		case WEST:
			newY = oldY - 1;
			break;
		default:
			MWError("Invalid Direction;");
	}

	RatIndexType ratIndex(1);
	for (ratIndex = RatIndexType(1); ratIndex.value() < MAX_RATS; ratIndex = RatIndexType(ratIndex.value() + 1)) {
		if (M->rat(ratIndex).playing &&
		    M->rat(ratIndex).x == newX &&
		    M->rat(ratIndex).y == newY) {
			printf("hit a rat!\n");
			sendMissileHit(M->rat(ratIndex).rat_id.value());
			M->hasMissileIs(FALSE);
			clearSquare(Loc(oldX), Loc(oldY));
			return;
		}
	}

	if (!M->maze_[newX][newY]) {
		M->xMissileIs(Loc(newX));
		M->yMissileIs(Loc(newY));
		showMissile(Loc(newX), Loc(newY), M->dirMissile(), Loc(oldX), Loc(oldY), true);
	} else {
		M->hasMissileIs(FALSE);
		clearSquare(Loc(oldX), Loc(oldY));
	}

	updateView = TRUE;
}

/* ----------------------------------------------------------------------- */

void DoViewUpdate()
{
	if (updateView) {	/* paint the screen */
		ShowPosition(MY_X_LOC, MY_Y_LOC, MY_DIR);
		if (M->peeking())
			ShowView(M->xPeek(), M->yPeek(), M->dirPeek());
		else
			ShowView(MY_X_LOC, MY_Y_LOC, MY_DIR);
		updateView = FALSE;
	}
}

/* ----------------------------------------------------------------------- */

/*
 * Sample code to send a packet to a specific destination
 */

/*
 * Notice the call to ConvertOutgoing.  You might want to call ConvertOutgoing
 * before any call to sendto.
 */

void sendPacketToPlayer(RatId ratId)
{
/*
	MW244BPacket pack;
	DataStructureX *packX;

	pack.type = PACKET_TYPE_X;
	packX = (DataStructureX *) &pack.body;
	packX->foo = d1;
	packX->bar = d2;

        ....

	ConvertOutgoing(pack);

	if (sendto((int)mySocket, &pack, sizeof(pack), 0,
		   (Sockaddr) destSocket, sizeof(Sockaddr)) < 0)
	  { MWError("Sample error") };
*/
}

/* ----------------------------------------------------------------------- */


/* ----------------------------------------------------------------------- */

/* This will presumably be modified by you.
   It is here to provide an example of how to open a UDP port.
   You might choose to use a different strategy
 */
void
netInit()
{
	Sockaddr		nullAddr;
	Sockaddr		*thisHost;
	char			buf[128];
	int				reuse;
	u_char          ttl;
	struct ip_mreq  mreq;

	/* MAZEPORT will be assigned by the TA to each team */
	M->mazePortIs(htons(MAZEPORT));

	gethostname(buf, sizeof(buf));
	if ((thisHost = resolveHost(buf)) == (Sockaddr *) NULL)
	  MWError("who am I?");
	bcopy((caddr_t) thisHost, (caddr_t) (M->myAddr()), sizeof(Sockaddr));

	M->theSocketIs(socket(AF_INET, SOCK_DGRAM, 0));
	if (M->theSocket() < 0)
	  MWError("can't get socket");

	/* SO_REUSEADDR allows more than one binding to the same
	   socket - you cannot have more than one player on one
	   machine without this */
	reuse = 1;
	if (setsockopt(M->theSocket(), SOL_SOCKET, SO_REUSEADDR, &reuse,
		   sizeof(reuse)) < 0) {
		MWError("setsockopt failed (SO_REUSEADDR)");
	}

	nullAddr.sin_family = AF_INET;
	nullAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	nullAddr.sin_port = M->mazePort();
	if (bind(M->theSocket(), (struct sockaddr *)&nullAddr,
		 sizeof(nullAddr)) < 0)
	  MWError("netInit binding");

	/* Multicast TTL:
	   0 restricted to the same host
	   1 restricted to the same subnet
	   32 restricted to the same site
	   64 restricted to the same region
	   128 restricted to the same continent
	   255 unrestricted

	   DO NOT use a value > 32. If possible, use a value of 1 when
	   testing.
	*/

	ttl = 1;
	if (setsockopt(M->theSocket(), IPPROTO_IP, IP_MULTICAST_TTL, &ttl,
		   sizeof(ttl)) < 0) {
		MWError("setsockopt failed (IP_MULTICAST_TTL)");
	}

	/* join the multicast group */
	mreq.imr_multiaddr.s_addr = htonl(MAZEGROUP);
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	if (setsockopt(M->theSocket(), IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)
		   &mreq, sizeof(mreq)) < 0) {
		MWError("setsockopt failed (IP_ADD_MEMBERSHIP)");
	}

	/*
	 * Now we can try to find a game to join; if none, start one.
	 */
	 
	printf("\n");

	/* Get the multi-cast address ready to use in SendData()
           calls. */
	memcpy(&groupAddr, &nullAddr, sizeof(Sockaddr));
	groupAddr.sin_addr.s_addr = htonl(MAZEGROUP);

}

void
sendPacket(MW244BPacket *packet) {
	if (sendto((int) M->theSocket(), (void *) packet, sizeof(MW244BPacket), 0,
		(sockaddr *) &groupAddr, sizeof(Sockaddr)) < 0)
		MWError("Send packet failed");
}

void
sendStateUpdate() {
	StateUpdate *su = new StateUpdate(M->myRatId().value(), seq[STATE_UPDATE]++,
				M->myName_, MY_X_LOC, MY_Y_LOC, MY_DIR, M->cloaked(), M->score().value());
	MW244BPacket *outPacket = new MW244BPacket();
	memcpy(outPacket, su, sizeof (StateUpdate));
	printf("send state update, id: %d, name: %s, x: %d, y: %d, score: %d\n", su->rat_id, su->name, su->xPos, su->yPos, su->score);
	sendPacket(outPacket);
	delete su;
	delete outPacket;
}

void
sendMissileHit(uint16_t victim_id) {
	MissileHit *mh = new MissileHit(M->myRatId().value(), seq[MISSILE_HIT]++,
				M->myName_, victim_id);
	MW244BPacket *outPacket = new MW244BPacket();
	memcpy(outPacket, mh, sizeof (StateUpdate));
	printf("send missile hit, victim id: %d\n", mh->victimID);
	sendPacket(outPacket);
	delete mh;
	delete outPacket;
}

void
sendHeartBeat() {
	timeval last, now;
	gettimeofday(&now, NULL);
	last = M->lastHeartBeat();
	if ((now.tv_sec - last.tv_sec) * 1000
		+ (now.tv_usec - last.tv_usec) / 1000 < HEARTBEAT_SPEED) return;

	M->lastHeartBeatIs(now);
	sendStateUpdate();
}

void
sendMissileHitACK(uint16_t id, int score) {
	MissileHitACK *mha = new MissileHitACK(M->myRatId().value(), seq[MISSILE_HIT_ACK]++, M->myName_, id, score);
	MW244BPacket *outPacket = new MW244BPacket();
	memcpy(outPacket, mha, sizeof (LeaveGame));
	printf("send ack id: %d, score: %d\n", mha->shooterID, mha->score);
	sendPacket(outPacket);
	delete mha;
	delete outPacket;
}

void
sendLeaveGame() {
	LeaveGame *lg = new LeaveGame(M->myRatId().value(), seq[LEAVE_GAME]++, M->myName_);
	MW244BPacket *outPacket = new MW244BPacket();
	memcpy(outPacket, lg, sizeof (LeaveGame));
	printf("send leave game, id: %d, name: %s\n", lg->rat_id, lg->name);
	sendPacket(outPacket);
	delete lg;
	delete outPacket;
}

void
processPacket(MWEvent *eventPacket) {
	MW244BPacket *packet = eventPacket->eventDetail;
	PacketHeader *p = (PacketHeader *) packet;

	printf("type: %d, rat_id: %d, seq_id: %d, name: %s", p->type, p->rat_id, p->seq_id, p->name);
	if (p->rat_id == M->myRatId().value()) {
		printf("Packet from self, ignored\n");
		return;
	}

	switch (p->type) {
	case STATE_UPDATE:
		processStateUpdate(p);
		break;

	case MISSILE_HIT:
		processMissileHit(p);
		break;

	case MISSILE_HIT_ACK:
		processMissileHitACK(p);
		break;

	case LEAVE_GAME:
		processLeaveGame(p);
		break;

	default:
		MWError("Invalid incoming packet type\n");
		break;
	}
}

void joinGame() {
       MWEvent         event;
       MW244BPacket    incoming;
       timeval         base, now;
       RatIndexType    ratIndex(0);

       event.eventDetail = &incoming;
       gettimeofday(&base, NULL);

       while(1) {
               gettimeofday(&now, NULL);
               if ((now.tv_sec - base.tv_sec) * 1000 
                       + (now.tv_usec - base.tv_usec) / 1000 > 5000) {
			RatId rid(random() & (0x00ff));
			printf("ratid is %d\n", rid.value());
			M->myRatIdIs(rid);
			M->cloakedIs(0);
			int i = 0;
			for (i = 0; i < PACKET_TYPE; i++) seq[i] = 0;
			M->lastHeartBeatIs(now);
			Rat r;
			r.rat_id = M->myRatId();
			r.playing = TRUE;
			r.cloaked = FALSE;
			r.x = M->xloc();
			r.y = M->yloc();
			r.dir = M->dir();
			r.score = M->score();
			strncpy(r.name, M->myName_, MAXNAMELEN);
			M->ratIs(r, RatIndexType(0));
			return;
               }
               
               NextEvent(&event, M->theSocket());
               if (event.eventType == EVENT_NETWORK) {
			PacketHeader *p = (PacketHeader *) event.eventDetail;
			if (p->type == STATE_UPDATE) {
				processPacket(&event);
			}
               }
       }
}

void
processStateUpdate(PacketHeader *packet) {
	StateUpdate *su = (StateUpdate *)packet;
	assert(su->rat_id != M->myRatId().value());

	RatIndexType ratIndex(1);
	
	for (ratIndex = RatIndexType(1); ratIndex.value() < MAX_RATS; ratIndex = RatIndexType(ratIndex.value() + 1))
		if (su->rat_id == M->rat(ratIndex).rat_id.value()) break;

	if (ratIndex < MAX_RATS) {
		Rat r = M->rat(ratIndex);
		if (su->seq_id <= r.seq[STATE_UPDATE]) {
			printf("Reversed state update packet, drop\n");
			return;
		}
		r.cloaked = (su->cloaked != 0);
		r.x = Loc(su->xPos);
		r.y = Loc(su->yPos);
		r.dir = Direction(su->dir);
		r.score = Score(su->score);
		r.seq[STATE_UPDATE] = su->seq_id;
		strncpy(r.name, su->name, MAXNAMELEN);
		M->ratIs(r, ratIndex);
	} else {
		for (ratIndex = RatIndexType(0); ratIndex.value() < MAX_RATS; ratIndex = RatIndexType(ratIndex.value() + 1))
			if (!M->rat(ratIndex).playing) break;
		if (ratIndex == MAX_RATS) {
			printf("game full\n");
			return;
		}
		Rat r;
		r.rat_id = su->rat_id;
		r.playing = TRUE;
		r.cloaked = (su->cloaked != 0);
		r.x = Loc(su->xPos);
		r.y = Loc(su->yPos);
		r.dir = Direction(su->dir);
		r.score = Score(su->score);
		int i = 0;
		for (i = 0; i < PACKET_TYPE; i++) r.seq[i] = 0;
		r.seq[STATE_UPDATE] = su->seq_id;
		M->ratIs(r, ratIndex);
	}
	UpdateScoreCard(ratIndex);
}

void
processMissileHit(PacketHeader *packet) {
	MissileHit *mh = (MissileHit *)packet;

	int minus = M->cloaked() ? 7 : 5;
	int plus = M->cloaked() ? 13 : 11;
	if (mh->victimID == M->myRatId().value()) {
		printf("I'm victim\n");
		M->scoreIs(Score(M->score().value() - minus));
		UpdateScoreCard(MY_RAT_INDEX);
		sendMissileHitACK(mh->rat_id, plus);
	}
}

void
processMissileHitACK(PacketHeader *packet) {

}

void
processLeaveGame(PacketHeader *packet) {
	LeaveGame *lg = (LeaveGame *)packet;
	RatIndexType ratIndex(0);

	for (ratIndex = RatIndexType(0); ratIndex.value() < MAX_RATS; ratIndex = RatIndexType(ratIndex.value() + 1))
		if (lg->rat_id == M->rat(ratIndex).rat_id.value()) break;

	if (ratIndex.value() == MAX_RATS) {
		printf("unknown rat, ignore\n");
		return;
	}

	Rat r = M->rat(ratIndex);
	r.playing = FALSE;
	M->ratIs(r, ratIndex);
}

/* ----------------------------------------------------------------------- */
