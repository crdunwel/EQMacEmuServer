#ifndef QUARMSERVER_BULK_INSERT_QUEUE_H
#define QUARMSERVER_BULK_INSERT_QUEUE_H

#endif //QUARMSERVER_BULK_INSERT_QUEUE_H

/*	EQEMu: Everquest Server Emulator
	Copyright (C) 2001-2008 EQEMu Development Team (http://eqemulator.net)

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; version 2 of the License.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY except by those people which sell it, which
	are required to give you total support for your newly bought product;
	without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE. See the GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA

*/

#include "../common/servertalk.h"
#include "../world/worlddb.h"
#include "../common/strings.h"

#include <string>
#include <deque>
#include <vector>

template<typename T>
struct DequeWithTable {
	std::deque<T> deque;
	std::string tableName;
};

class BulkInsertManager {
public:
	BulkInsertManager(const size_t _maxRecords);
	~BulkInsertManager();

	bool isQueueFull() { return totalRecordCount >= maxRecords; };
	void writeQueueToDatabase();
	vector<std::string> getSQLStatements();

	size_t getTotalRecordCount() { return totalRecordCount; };
	void setMaxRecordSize(size_t size) { maxRecords = size };

	void addServerSpeechRecord(const Server_Speech_Struct& record);
	void addPlayerLogItemDeleteRecord(const QSPlayerLogItemDelete_Struct& record);
	void addPlayerLogItemMoveRecord(const QSPlayerLogItemMove_Struct& record);
	void addMerchantLogTransactionRecord(const QSMerchantLogTransaction_Struct& record);
	void addPlayerAARateHourlyRecord(const QSPlayerAARateHourly_Struct& record);
	void addPlayerAAPurchaseRecord(const QSPlayerAAPurchase_Struct& record);
	void addPlayerTSEventsRecord(const QSPlayerTSEvents_Struct& record);
	void addPlayerQGlobalUpdateRecord(const QSPlayerQGlobalUpdate_Struct& record);
	void addPlayerLootRecordsRecord(const QSPlayerLootRecords_struct& record);
	void addServerPacketRecord(const ServerPacket& sqlStatement);

	template<typename T>
	void addSQLInsertStatement(std::vector<std::string>& statements, std::deque<T>& records, const std::string& tableName);
	std::string createBulkInsertSQLStatement(const std::string& tableName, const std::deque<std::string>& records);

	std::string convertRecordToSQLValues(const Server_Speech_Struct* record);
	std::string convertRecordToSQLValues(const QSPlayerLogItemDelete_Struct* record);
	std::string convertRecordToSQLValues(const QSPlayerLogItemMove_Struct* record);
	std::string convertRecordToSQLValues(const QSMerchantLogTransaction_Struct* record);
	std::string convertRecordToSQLValues(const QSPlayerAARateHourly_Struct* record);
	std::string convertRecordToSQLValues(const QSPlayerAAPurchase_Struct* record);
	std::string convertRecordToSQLValues(const QSPlayerTSEvents_Struct* record);
	std::string convertRecordToSQLValues(const QSPlayerQGlobalUpdate_Struct* record);
	std::string convertRecordToSQLValues(const QSPlayerLootRecords_struct* record);
	std::string convertRecordToSQLValues(const ServerPacket* record);

private:
	size_t maxRecords;
	size_t totalRecordCount;

	std::deque<Server_Speech_Struct> serverSpeechRecords;
	std::deque<QSPlayerLogItemDelete_Struct> playerLogItemDeleteRecords;
	std::deque<QSPlayerLogItemMove_Struct> playerLogItemMoveRecords;
	std::deque<QSMerchantLogTransaction_Struct> merchantLogTransactionRecords;
	std::deque<QSPlayerAARateHourly_Struct> playerAARateHourlyRecords;
	std::deque<QSPlayerAAPurchase_Struct> playerAAPurchaseRecords;
	std::deque<QSPlayerTSEvents_Struct> playerTSEventsRecords;
	std::deque<QSPlayerQGlobalUpdate_Struct> playerQGlobalUpdateRecords;
	std::deque<QSPlayerLootRecords_struct> playerLootRecordsRecords;
	std::deque<ServerPacket> serverPacketRecords;
};

#endif
