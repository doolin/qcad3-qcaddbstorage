#ifndef RS_DBSTORAGE_H
#define RS_DBSTORAGE_H

#include "RS_PassiveTransaction"
#include "RS_Storage"
#include "RS_DbClient"



/**
 * A DB storage object implements the storage interface to store 
 * document data in an SQLite DB.
 *
 * \author Andrew Mustun
 * \ingroup qcaddbstorage
 */
class RS_DbStorage : public RS_Storage {
public:
    RS_DbStorage();
    virtual ~RS_DbStorage();

    virtual std::list<RS_Entity::Id> queryAll();
    virtual RS_Entity* queryEntity(RS_Entity::Id entityId);
    
    virtual void save(RS_Entity& entity);
    virtual void deleteEntity(RS_Entity::Id entityId);
    
    virtual void beginTransaction();
    virtual void commitTransaction();
    
    virtual int getLastTransactionId();
    virtual void setLastTransactionId(int transactionId);
    virtual void saveTransaction(RS_ActiveTransaction& transaction);;
    virtual void deleteTransactionsFrom(int transactionId);
    virtual RS_PassiveTransaction getTransaction(int transactionId);
    virtual int getMaxTransactionId();

    virtual void toggleUndoStatus(std::list<RS_Entity::Id>& entities);

protected:
    RS_Entity* queryEntity(RS_Entity::Id entityId, RS_Entity::TypeId typeId);
    RS_Entity::TypeId getEntityType(RS_Entity::Id entityId);

private:
    //! connection to SQLite DB:
    RS_DbConnection db;
};

#endif
