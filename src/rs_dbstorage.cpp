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
            "selectionStatus INTEGER, "
            "undoStatus INTEGER, "
            "minX REAL, "
            "minY REAL, "
            "minZ REAL, "
            "maxX REAL, "
            "maxY REAL, "
            "maxZ REAL"
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



std::set<RS_Entity::Id> RS_DbStorage::queryAll() {
    std::set<RS_Entity::Id> ret;
			
    RS_DbCommand cmd(
        db, 
        "SELECT id "
        "FROM Entity "
        "WHERE BlockName IS NULL"
    );

	RS_DbReader reader = cmd.executeReader();
	while (reader.read()) {
        ret.insert(reader.getInt64(0));
    }

    return ret;
}



std::set<RS_Entity::Id> RS_DbStorage::querySelected() {
    std::set<RS_Entity::Id> ret;
			
    RS_DbCommand cmd(
        db, 
        "SELECT id "
        "FROM Entity "
        "WHERE BlockName IS NULL "
        "   AND selectionStatus=1"
    );

	RS_DbReader reader = cmd.executeReader();
	while (reader.read()) {
        ret.insert(reader.getInt64(0));
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

    RS_Entity::TypeId typeId = (RS_Entity::TypeId)reader.getInt64(0);
        
    return queryEntity(entityId, typeId);
}
    
    

/**
 * Internal function to query an entity if the type is already known.
 */
RS_Entity* RS_DbStorage::queryEntity(RS_Entity::Id entityId, RS_Entity::TypeId typeId) {
    RS_DbsEntity* dbEntity = RS_DbsEntityRegistry::getDbEntity(typeId);
    if (dbEntity==NULL) {
        RS_Debug::error("RS_DbStorage::queryEntity: "
            "no DB Entity object registered for entity type %d", typeId);
        return NULL;
    }
    return dbEntity->instantiate(db, entityId);
}



void RS_DbStorage::clearSelection(std::set<RS_Entity::Id>* affectedEntities) {
    // find out which entities will be affected:
    if (affectedEntities!=NULL) {
        RS_DbCommand cmd(
            db, 
            "SELECT id "
            "FROM Entity "
            "WHERE selectionStatus=1"
        );
        RS_DbReader reader = cmd.executeReader();
        while (reader.read()) {
            affectedEntities->insert(reader.getInt64(0));
        }
    }

    RS_DbCommand cmd(
        db, 
        "UPDATE Entity "
        "SET selectionStatus=0 "
        "WHERE selectionStatus=1"
    );
    cmd.executeNonQuery();
}



void RS_DbStorage::selectEntity(
    RS_Entity::Id entityId, bool add, 
    std::set<RS_Entity::Id>* affectedEntities) {

    if (add) {
        // only the entity that is added to the selection is affected:
        if (affectedEntities!=NULL) {
            affectedEntities->insert(entityId);
        }

        RS_DbCommand cmd(
            db, 
            "UPDATE Entity "
            "SET selectionStatus=1 "
            "WHERE id=?"
        );
        cmd.bind(1, entityId);
        cmd.executeNonQuery();
    }
    else {
        // find out which entities will be affected:
        if (affectedEntities!=NULL) {
            RS_DbCommand cmd(
                db, 
                "SELECT id "
                "FROM Entity "
                "WHERE (id=? AND selectionStatus=0) OR "
                "      (id!=? AND selectionStatus=1)"
            );
            cmd.bind(1, entityId);
            cmd.bind(2, entityId);
	        RS_DbReader reader = cmd.executeReader();
            while (reader.read()) {
                affectedEntities->insert(reader.getInt64(0));
            }
        }

        RS_DbCommand cmd(
            db, 
            "UPDATE Entity "
            "SET selectionStatus=NOT(selectionStatus) "
            "WHERE (id=? AND selectionStatus=0) OR "
            "      (id!=? AND selectionStatus=1)"
        );
        cmd.bind(1, entityId);
        cmd.bind(2, entityId);
        cmd.executeNonQuery();
    }
}



void RS_DbStorage::selectEntities(
    std::set<RS_Entity::Id>& entityIds, 
    bool add, 
    std::set<RS_Entity::Id>* affectedEntities) {
        
    if (affectedEntities!=NULL) {
        (*affectedEntities) = entityIds;
    }

    //if (add) {
        // only the entities that are added to the selection are affected:
        //if (affectedEntities!=NULL) {
            //affectedEntities->insert(affectedEntities->begin(),myvector.begin(),myvector.end());
            //affectedEntities->insert(entityId);
        //    (*affectedEntities) = entityIds;
        //}

        /*
        RS_DbCommand cmd(
            db, 
            std::string(
                "UPDATE Entity "
                "SET selectionStatus=1 "
                "WHERE id IN "
            ) + getSqlList(entityIds)
        );
        //cmd.bind(1, entityId);
        cmd.executeNonQuery();
        */
    //}
    //else {
    if (!add) {
        // find out which entities will be deselected:
        if (affectedEntities!=NULL) {
            //(*affectedEntities) = entityIds;

            RS_DbCommand cmd(
                db, 
                "SELECT id "
                "FROM Entity "
                "WHERE selectionStatus=1"
            );
	        RS_DbReader reader = cmd.executeReader();
            while (reader.read()) {
                affectedEntities->insert(reader.getInt64(0));
            }
        }

        // deselect all:
        RS_DbCommand cmd(
            db, 
            "UPDATE Entity "
            "SET selectionStatus=0 "
            "WHERE selectionStatus=1"
        );
        cmd.executeNonQuery();
    }

    // select given entities:
    RS_DbCommand cmd(
        db, 
        std::string(
            "UPDATE Entity "
            "SET selectionStatus=1 "
            "WHERE id IN "
        ) + getSqlList(entityIds)
    );
    cmd.executeNonQuery();
}



RS_Box RS_DbStorage::getBoundingBox() {
    RS_DbCommand cmd(
        db, 
        "SELECT MIN(minX), MIN(minY), MIN(minZ), "
        "       MAX(maxX), MAX(maxY), MAX(maxZ) "
        "FROM Entity "
        "WHERE undoStatus=0"
    );
    RS_DbReader reader = cmd.executeReader();

    RS_Vector minV;
    RS_Vector maxV;
    
    if (reader.read()) {
        minV.x = reader.getDouble(0);
        minV.y = reader.getDouble(1);
        minV.z = reader.getDouble(2);
        
        maxV.x = reader.getDouble(3);
        maxV.y = reader.getDouble(4);
        maxV.z = reader.getDouble(5);
    }

    return RS_Box(minV, maxV);
}



int RS_DbStorage::getLastTransactionId() {
    RS_DbCommand cmd(
        db, 
        "SELECT lastTransaction "
        "FROM LastTransaction"
    );

    return cmd.executeInt();
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
        "INSERT INTO Entity VALUES(?,?,?,?,?,?,?,?,?,?,?,?);"
    );

    cmd.bind(1);
    cmd.bind(2, entity.getTypeId());
    cmd.bind(3);
    cmd.bind(4);
    cmd.bind(5, entity.isSelected());
    cmd.bind(6, 0);

    RS_Box boundingBox = entity.getBoundingBox();
    RS_Vector c1 = boundingBox.getDefiningCorner1();
    RS_Vector c2 = boundingBox.getDefiningCorner2();
    cmd.bind(7, c1.x);
    cmd.bind(8, c1.y);
    cmd.bind(9, c1.z);
    cmd.bind(10, c2.x);
    cmd.bind(11, c2.y);
    cmd.bind(12, c2.z);

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

    // store the set of entities that are affected by the transaction:
    std::set<RS_Entity::Id> affectedEntities = transaction.getAffectedEntities();
    std::set<RS_Entity::Id>::iterator it;
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

    // look up set of affected entities:
    RS_DbCommand cmd2(
        db, 
        "SELECT eid "
        "FROM TransactionEntity "
        "WHERE tid=?"
    );
    cmd2.bind(1, transactionId);

    std::set<RS_Entity::Id> affectedEntities;

	RS_DbReader reader = cmd2.executeReader();
	while (reader.read()) {
        affectedEntities.insert(reader.getInt64(0));
        RS_Debug::debug("RS_Transaction::RS_Transaction: "
            "affected entity: %d", reader.getInt64(0));
    }

    return RS_PassiveTransaction(*this, transactionId, text, affectedEntities);
}
    
    
    
void RS_DbStorage::deleteTransactionsFrom(int transactionId) {
    RS_Debug::debug("RS_DbStorage::deleteTransactionsFrom: transactionId: %d", transactionId);

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
        // check if there are transactions we are keeping which still refer to the
        // entity in question:
        RS_DbCommand cmd4(
            db, 
            "SELECT eid "
            "FROM TransactionEntity "
            "WHERE tid<? AND eid=?"
        );
        cmd4.bind(1, transactionId);
        cmd4.bind(2, reader.getInt64(0));
        RS_DbReader reader4 = cmd4.executeReader();
        if (reader4.read()==false) {
        //if (getUndoStatus(reader.getInt64(0))==true) {
            RS_Debug::debug("RS_DbStorage::deleteTransactionsFrom: deleteEntity: %d", reader.getInt64(0));
            deleteEntity(reader.getInt64(0));
        }
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
        "SELECT max(id) "
        "FROM TransactionLog"
    );
    return cmd.executeInt();
}
    
    
    
void RS_DbStorage::toggleUndoStatus(std::set<RS_Entity::Id>& entities) {
    std::set<RS_Entity::Id>::iterator it;
    for (it=entities.begin(); it!=entities.end(); it++) {
        RS_Debug::debug("RS_DbStorage::undo: toggle entity: %d", *it);

        toggleUndoStatus(*it);
    }
}



void RS_DbStorage::toggleUndoStatus(RS_Entity::Id entityId) {
    RS_DbCommand cmd(
        db, 
        "UPDATE Entity "
        "SET undoStatus=NOT(undoStatus) "
        "WHERE id=?"
    );
    cmd.bind(1, entityId);
    cmd.executeNonQuery();
}



bool RS_DbStorage::getUndoStatus(RS_Entity::Id entityId) {
    RS_DbCommand cmd(
        db, 
        "SELECT undoStatus "
        "FROM Entity "
        "WHERE id=?"
    );
    cmd.bind(1, entityId);
    return (bool)cmd.executeInt();
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



/**
 * Helper function that turns the given list of IDs into an SQL
 * string list.
 *
 * \return List of IDs as string for use in SQL queries. 
 *      E.g. "(1,7,5,17)"
 */
std::string RS_DbStorage::getSqlList(std::set<RS_Entity::Id>& values) {
    std::stringstream ss;
    ss << "(";
    std::set<RS_Entity::Id>::iterator it;
    for (it=values.begin(); it!=values.end(); ++it) {
        if (it!=values.begin()) {
            ss << ",";
        }
        ss << (*it);
    }
    ss << ")";
    return ss.str();
}
