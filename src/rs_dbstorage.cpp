#include "RS_Debug"
#include "RS_DbStorage"
#include "RS_DbException"
#include "RS_DbsEntity"
#include "RS_DbsEntityRegistry"



/**
 * Sets up a DB for the document and initializes the tables.
 */
RS_DbStorage::RS_DbStorage() {
    // creates the DB in a private temporary file on disk (for debugging):
    //db.open("g.db");
    
    // creates the DB in memory (production):
    db.open(":memory:");

    db.executeNonQuery(
        "CREATE TABLE Entity("
            "id INTEGER PRIMARY KEY, "
            "type INTEGER, "
            "blockName VARCHAR, "
            "layerName VARCHAR, "
            "undoStatus INTEGER"
        ");"
    );

    db.executeNonQuery(
        "CREATE TABLE Block("
            "name VARCHAR PRIMARY KEY"
        ");"
    );
    
    db.executeNonQuery(
        "CREATE TABLE Layer("
            "name VARCHAR PRIMARY KEY"
        ");"
    );
    
    db.executeNonQuery(
        "CREATE TABLE TransactionLog("
            "id INTEGER PRIMARY KEY, "
            "text VARCHAR"
        ");"
    );
    
    db.executeNonQuery(
        "CREATE TABLE TransactionEntity("
            "tid INTEGER, "
            "eid INTEGER, "
            "PRIMARY KEY(tid, eid)"
        ");"
    );
    
    db.executeNonQuery(
        "CREATE TABLE LastTransaction("
            "lastTransaction INTEGER PRIMARY KEY"
        ");"
    );
    
    RS_DbCommand cmd(
        db, 
        "INSERT INTO LastTransaction VALUES(?);"
    );
    cmd.bind(1, -1);
	cmd.executeNonQuery();
 
    RS_DbsEntityRegistry::initDb(db);
}



/**
 * Closes the DB connection.
 */
RS_DbStorage::~RS_DbStorage() {
    db.close();
}



std::list<RS_Entity::Id> RS_DbStorage::queryAll() {
    std::list<RS_Entity::Id> ret;
			
    RS_DbCommand cmd(
        db, 
        "SELECT id "
        "FROM Entity "
        "WHERE BlockName IS NULL"
    );

	RS_DbReader reader = cmd.executeReader();
	while (reader.read()) {
        ret.push_back(reader.getInt64(0));
    }

    return ret;
}



RS_Entity* RS_DbStorage::queryEntity(RS_Entity::Id entityId) {
    // query entity type:
    RS_DbCommand cmd(
        db, 
        "SELECT type "
        "FROM Entity "
        "WHERE id=? AND undoStatus=0"
    );
    cmd.bind(1, entityId);

    RS_DbReader reader = cmd.executeReader();
	if (!reader.read()) {
        return NULL;
    }

    RS_Entity::TypeId typeId = reader.getInt64(0);
        
    return queryEntity(entityId, typeId);
}



/**
 * Internal function to query an entity if the type is already known.
 */
RS_Entity* RS_DbStorage::queryEntity(RS_Entity::Id entityId, RS_Entity::TypeId typeId) {
    RS_DbsEntity* dbEntity = RS_DbsEntityRegistry::getDbEntity(typeId);
    if (dbEntity==NULL) {
        RS_Debug::error("RS_DbStorage::queryEntity: "
            "no dbEntity object registered for entity type %d", typeId);
        return NULL;
    }
    return dbEntity->instantiate(db, entityId);
}



int RS_DbStorage::getLastTransactionId() {
    RS_DbCommand cmd(
        db, 
        "SELECT lastTransaction "
        "FROM LastTransaction"
    );

    return cmd.executeInt64();
}



void RS_DbStorage::setLastTransactionId(int tid) {
    RS_DbCommand cmd(
        db, 
        "UPDATE LastTransaction "
        "SET lastTransaction=?"
    );
    cmd.bind(1, tid);
	cmd.executeNonQuery();
}

    
    
    
void RS_DbStorage::save(RS_Entity& entity) {
    // generic entity information has to be stored for all entity types:
    RS_DbCommand cmd(
        db, 
        "INSERT INTO Entity VALUES(?,?,?,?,?);"
    );
    cmd.bind(1);
    cmd.bind(2, entity.getTypeId());
    cmd.bind(3);
    cmd.bind(4);
    cmd.bind(5, 0);
	cmd.executeNonQuery();
    entity.setId(db.getLastInsertedRowId());
    
    // look up storage object in entity registry:
    RS_DbsEntity* dbEntity = RS_DbsEntityRegistry::getDbEntity(entity.getTypeId());

    // store entity:
    dbEntity->save(db, entity);
}
    
    
    
void RS_DbStorage::beginTransaction() {
    db.startTransaction();
}



void RS_DbStorage::commitTransaction() {
    db.endTransaction();
}
    
    
    
void RS_DbStorage::saveTransaction(RS_ActiveTransaction& transaction) {
    // store the transaction in the transaction log:
    RS_DbCommand cmd(
        db, 
        "INSERT INTO TransactionLog VALUES(?,?)"
    );
    cmd.bind(1, transaction.getId());
    cmd.bind(2, transaction.getText());
	cmd.executeNonQuery();

    // store the list of entities that are affected by the transaction:
    std::list<RS_Entity::Id> affectedEntities = transaction.getAffectedEntities();
    std::list<RS_Entity::Id>::iterator it;
    for (it=affectedEntities.begin(); it!=affectedEntities.end(); ++it) {
        RS_DbCommand cmd(
            db, 
            "INSERT INTO TransactionEntity VALUES(?,?);"
        );
        cmd.bind(1, transaction.getId());
        cmd.bind(2, *it);
        cmd.executeNonQuery();
    }
}

    
    
RS_PassiveTransaction RS_DbStorage::getTransaction(int transactionId) {
    // look up transaction:
    RS_DbCommand cmd1(
        db, 
        "SELECT text "
        "FROM TransactionLog "
        "WHERE id=?"
    );
    
    std::string text;
    try {
        text = cmd1.executeString();
    }
    catch (RS_DbException e) {
        text = "";
    }

    // look up list of affected entities:
    RS_DbCommand cmd2(
        db, 
        "SELECT eid "
        "FROM TransactionEntity "
        "WHERE tid=?"
    );
    cmd2.bind(1, transactionId);

    std::list<RS_Entity::Id> affectedEntities;

	RS_DbReader reader = cmd2.executeReader();
	while (reader.read()) {
        affectedEntities.push_back(reader.getInt64(0));
        RS_Debug::debug("RS_Transaction::RS_Transaction: "
            "affected entity: %d", reader.getInt64(0));
    }

    return RS_PassiveTransaction(*this, transactionId, text, affectedEntities);
}
    
    
    
void RS_DbStorage::deleteTransactionsFrom(int transactionId) {
    // delete orphaned entities:
    RS_DbCommand cmd3(
        db, 
        "SELECT eid "
        "FROM TransactionEntity "
        "WHERE tid>=?"
    );
    cmd3.bind(1, transactionId);
	RS_DbReader reader = cmd3.executeReader();
	while (reader.read()) {
        deleteEntity(reader.getInt64(0));
    }

    // delete records of affected entities of the transaction:
    RS_DbCommand cmd(
        db, 
        "DELETE FROM TransactionEntity "
        "WHERE tid>=?"
    );
    cmd.bind(1, transactionId);
    cmd.executeNonQuery();
    
    // delete transaction from transaction log:
    RS_DbCommand cmd2(
        db, 
        "DELETE FROM TransactionLog "
        "WHERE id>=?"
    );
    cmd2.bind(1, transactionId);
    cmd2.executeNonQuery();
}



int RS_DbStorage::getMaxTransactionId() {
    RS_DbCommand cmd(
        db, 
        "SELECT max(id) FROM TransactionLog"
    );
    return cmd.executeInt();
}
    
    
    
void RS_DbStorage::toggleUndoStatus(std::list<RS_Entity::Id>& entities) {
    std::list<RS_Entity::Id>::iterator it;
    for (it=entities.begin(); it!=entities.end(); it++) {
        RS_Debug::debug("RS_DbStorage::undo: toggle entity: %d", *it);

        RS_DbCommand cmd(
            db, 
            "UPDATE Entity "
            "SET undoStatus=NOT(undoStatus) "
            "WHERE id=?"
        );
        cmd.bind(1, *it);
    	cmd.executeNonQuery();
    }
}
    
    
    
void RS_DbStorage::deleteEntity(RS_Entity::Id entityId) {
    RS_Entity::TypeId typeId = getEntityType(entityId);

    RS_DbsEntityRegistry::deleteEntity(db, typeId, entityId);

    RS_DbCommand cmd(
        db, 
        "DELETE FROM Entity "
        "WHERE id=?"
    );
    cmd.bind(1, entityId);
    cmd.executeNonQuery();
}
    

    
RS_Entity::TypeId RS_DbStorage::getEntityType(RS_Entity::Id entityId) {
    RS_DbCommand cmd(
        db, 
        "SELECT type "
        "FROM Entity "
        "WHERE id=?"
    );
    cmd.bind(1, entityId);
    return cmd.executeInt();
}
