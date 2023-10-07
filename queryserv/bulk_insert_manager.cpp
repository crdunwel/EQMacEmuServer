/*	EQEMu: Everquest Server Emulator
	Copyright (C) 2001-2002 EQEMu Development Team (http://eqemu.org)

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

#include "bulk_insert_manager.h"
#include "../world/worlddb.h"
#include "../common/strings.h"
#include "../common/servertalk.h"

BulkInsertManager::BulkInsertManager(const size_t _maxRecords) {
	maxRecords = _maxRecords;
	totalRecordCount = 0;

	serverSpeechRecords.resize(maxRecords);
	playerLogItemDeleteRecords.resize(maxRecords);
	playerLogItemMoveRecords.resize(maxRecords);
	merchantLogTransactionRecords.resize(maxRecords);
	playerAARateHourlyRecords.resize(maxRecords);
	playerAAPurchaseRecords.resize(maxRecords);
	playerTSEventsRecords.resize(maxRecords);
	playerQGlobalUpdateRecords.resize(maxRecords);
	playerLootRecordsRecords.resize(maxRecords);
	serverPacketRecords.resize(maxRecords);
}

void BulkInsertManager::writeQueueToDatabase() {
	std::vector<std::string> statements = getSQLStatements();
	database.TransactionBulkQueryDatabase(statements, true)
	resetTimer();
}

std::vector<std::string> BulkInsertManager::getSQLStatements() {
	std::vector<std::string> statements;

	addSQLInsertStatement(statements, serverSpeechRecords, "qs_player_speech");
	addSQLInsertStatement(statements, playerLogItemDeleteRecords, "qs_player_item_delete_log");
	addSQLInsertStatement(statements, playerLogItemMoveRecords, "qs_player_item_move_log");
	addSQLInsertStatement(statements, merchantLogTransactionRecords, "qs_merchant_transaction_log");
	addSQLInsertStatement(statements, playerAARateHourlyRecords, "qs_player_aa_rate_hourly");
	addSQLInsertStatement(statements, playerAAPurchaseRecords, "qs_player_aa_purchase_log");
	addSQLInsertStatement(statements, playerTSEventsRecords, "qs_player_ts_event_log");
	addSQLInsertStatement(statements, playerQGlobalUpdateRecords, "qs_player_qglobal_updates_log");
	addSQLInsertStatement(statements, playerLootRecordsRecords, "qs_player_loot_records_log");
	// To add more, just add a line like the ones above and
	// ensure the deque for that type and the appropriate table name is provided.

	for (const auto& query : serverPacketRecords) {
		statements.push_back(convertRecordToSQLValues(&query));
	}
	serverPacketRecords.clear();

	return statements;
}

template<typename T>
void BulkInsertManager::addSQLInsertStatement(std::vector<std::string>& statements, std::deque<T>& records, const std::string& tableName) {
	std::vector<std::string> values;
	for (const auto& record : records) {
		values.push_back(convertRecordToSQLValue(&record));
	}
	records.clear(); // no longer need
	std::string insertStatement = createBulkInsertSQLStatement(tableName, values);
	if (!insertStatement.empty()) {
		statements.push_back(insertStatement);
	}
}

std::string BulkInsertManager::createBulkInsertSQLStatement(const std::string& tableName, const std::deque<std::string>& records) {
	if (records.empty()) {
		return "";
	}

	// Concatenate all records with comma separators
	std::string concatenatedValues = std::accumulate(std::next(records.begin()), records.end(),
	                                                 records[0], // start with the first value
	                                                 [](const std::string& a, const std::string& b) {
		                                                 return a + "," + b;
	                                                 }
	);

	return "INSERT INTO `" + tableName + "` VALUES " + concatenatedValues + ";";
}

bool BulkInsertManager::isTimerUp() const {
	return std::chrono::steady_clock::now() - lastResetTime >= timerDuration;
}

void BulkInsertManager::resetTimer() {
	lastResetTime = std::chrono::steady_clock::now();
}

std::string BulkInsertManager::convertRecordToSQLValues(const Server_Speech_Struct* record) {
	if (!record) {
		return "";
	}

	char escapedFrom[strlen(record->from) * 2 + 1];
	char escapedTo[strlen(record->to) * 2 + 1];
	char escapedMessage[strlen(record->message) * 2 + 1];

	database.DoEscapeString(escapedFrom, record->from, strlen(record->from));
	database.DoEscapeString(escapedTo, record->to, strlen(record->to));
	database.DoEscapeString(escapedMessage, record->message, strlen(record->message));

	return StringFormat("('%s', '%s', '%s', %i, %i, %i)",
	                    escapedFrom, escapedTo, escapedMessage,
	                    record->minstatus, record->guilddbid, record->type);
}

std::string BulkInsertManager::convertRecordToSQLValues(const QSPlayerLogItemDelete_Struct* record) {
	if (!record) {
		return "";
	}
	if (record->char_count == 0) {
		return "";
	}

	return StringFormat(
			"('%i', '%i', '%i', '%i', '%i', '%i', now())",
			record->char_id,
			record->char_slot,
			record->item_id,
			record->charges,
			record->stack_size,
			record->char_count
	);
}

std::string BulkInsertManager::convertRecordToSQLValues(const QSPlayerLogItemMove_Struct* record) {
	if (!record) {
		return "";
	}

	if (record->char_count == 0) {
		return "";
	}

	return StringFormat(
			"('%i', '%i', '%i', '%i', '%i', '%i', '%i', '%i')",
			record->char_id,
			record->items[itemIndex].from_slot,
			record->items[itemIndex].to_slot,
			record->items[itemIndex].item_id,
			record->items[itemIndex].charges,
			record->stack_size,
			record->char_count,
			record->postaction
	);
}

std::string BulkInsertManager::convertRecordToSQLValues(const QSMerchantLogTransaction_Struct* record) {
	if (!record) {
		return "";
	}
	if (record->char_count + record->merchant_count == 0) {
		return "";
	}

	return StringFormat(
			"('%i', '%i', '%i', '%i', '%i', '%i', '%i', '%i', '%i', '%i', '%i', '%i', '%i', '%i', '%i', '%i', NOW())",
			record->char_id,
			record->char_slot,
			record->item_id,
			record->charges,
			record->zone_id,
			record->merchant_id,
			record->merchant_money.platinum,
			record->merchant_money.gold,
			record->merchant_money.silver,
			record->merchant_money.copper,
			record->merchant_count,
			record->char_money.platinum,
			record->char_money.gold,
			record->char_money.silver,
			record->char_money.copper,
			record->char_count
	);
}

std::string BulkInsertManager::convertRecordToSQLValues(const QSPlayerAARateHourly_Struct* record) {
	if (!record) {
		return "";
	}
	if (record->charid == 0) {
		return "";
	}

	return StringFormat(
			"(%i, %i, UNIX_TIMESTAMP() - MOD(UNIX_TIMESTAMP(), 3600))",
			record->charid,
			record->add_points
	);
}

std::string BulkInsertManager::convertRecordToSQLValues(const QSPlayerAAPurchase_Struct* record) {
	if (!record) {
		return "";
	}
	if (record->charid == 0) {
		return "";
	}

	return StringFormat(
			"(%i, '%s', '%s', %i, %i, %i)",
			record->charid,
			record->aatype,
			record->aaname,
			record->aaid,
			record->cost,
			record->zone_id
	);
}

std::string BulkInsertManager::convertRecordToSQLValues(const QSPlayerTSEvents_Struct* record) {
	if (!record) {
		return "";
	}
	if (record->charid == 0) {
		return "";
	}

	return StringFormat(
			"(%i, %i, '%s', %i, %i, %i, %f)",
			record->charid,
			record->zone_id,
			record->results,
			record->recipe_id,
			record->tradeskill,
			record->trivial,
			record->chance
	);
}

std::string BulkInsertManager::convertRecordToSQLValues(const QSPlayerQGlobalUpdate_Struct* record) {
	if (!record) {
		return "";
	}
	if (record->charid == 0) {
		return "";
	}

	return StringFormat(
			"(%i, '%s', %i, '%s', '%s')",
			record->charid,
			Strings::Escape(record->action).c_str(),
			record->zone_id,
			record->varname,
			record->newvalue
	);
}

std::string BulkInsertManager::convertRecordToSQLValues(const QSPlayerLootRecords_struct* record) {
	if (!record) {
		return "";
	}
	if (record->charid == 0) {
		return "";
	}

	return StringFormat(
			"(%i, '%s', '%s', %i, %i, '%s', %i, %i, %i, %i, %i)",
			record->charid,
			Strings::Escape(record->corpse_name).c_str(),
			record->type,
			record->zone_id,
			record->item_id,
			Strings::Escape(record->item_name).c_str(),
			record->charges,
			record->money.platinum,
			record->money.gold,
			record->money.silver,
			record->money.copper
	);
}

std::string BulkInsertManager::convertRecordToSQLValues(const QSPlayerLootRecords_struct* record) {
	if (!record) {
		return "";
	}
	if (record->charid == 0) {
		return "";
	}

	return StringFormat(
			"(%i, '%s', '%s', %i, %i, '%s', %i, %i, %i, %i, %i)",
			record->charid,
			Strings::Escape(record->corpse_name).c_str(),
			record->type,
			record->zone_id,
			record->item_id,
			Strings::Escape(record->item_name).c_str(),
			record->charges,
			record->money.platinum,
			record->money.gold,
			record->money.silver,
			record->money.copper
	);
}

std::string BulkInsertManager::convertRecordToSQLValues(const ServerPacket* record) {
	if (!record) {
		return "";
	}

	auto queryBuffer = new char[record->ReadUInt32() + 1];
	record->ReadString(queryBuffer);

	std::string query(queryBuffer);
	delete[] queryBuffer;
	return query;
}

void BulkInsertManager::addServerSpeechRecord(const Server_Speech_Struct& record) {
	serverSpeechRecords.push_back(record);
	totalRecordCount++;
}

void BulkInsertManager::addPlayerLogItemDeleteRecord(const QSPlayerLogItemDelete_Struct& record) {
	playerLogDeleteRecords.push_back(record);
	totalRecordCount++;
}

void BulkInsertManager::addPlayerLogItemMoveRecord(const QSPlayerLogItemMove_Struct& record) {
	playerLogItemMoveRecords.push_back(record);
	totalRecordCount++;
}

void BulkInsertManager::addMerchantLogTransactionRecord(const QSMerchantLogTransaction_Struct& record) {
	merchantLogTransactionRecords.push_back(record);
	totalRecordCount++;
}

void BulkInsertManager::addPlayerAARateHourlyRecord(const QSPlayerAARateHourly_Struct& record) {
	playerAARateHourlyRecords.push_back(record);
	totalRecordCount++;
}

void BulkInsertManager::addPlayerAAPurchaseRecord(const QSPlayerAAPurchase_Struct& record) {
	playerAAPurchaseRecords.push_back(record);
	totalRecordCount++;
}

void BulkInsertManager::addPlayerTSEventsRecord(const QSPlayerTSEvents_Struct& record) {
	playerTSEventsRecords.push_back(record);
	totalRecordCount++;
}

void BulkInsertManager::addPlayerQGlobalUpdateRecord(const QSPlayerQGlobalUpdate_Struct& record) {
	playerQGlobalUpdateRecords.push_back(record);
	totalRecordCount++;
}

void BulkInsertManager::addPlayerLootRecordsRecord(const QSPlayerLootRecords_struct& record) {
	playerLootRecordsRecords.push_back(record);
	totalRecordCount++;
}

void BulkInsertManager::addServerPacketRecord(const ServerPacket& record) {
	serverPacketRecords.push_back(record);
	totalRecordCount++;
}
