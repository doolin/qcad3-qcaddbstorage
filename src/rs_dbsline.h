#include "RS_DbsEntity"
#include "RS_Line"



/**
 * Handles the DB storage for line entities.
 *
 * \author Andrew Mustun
 * \ingroup qcaddbstorage
 */
class RS_DbsLine : public RS_DbsEntity {
public:
    RS_DbsLine() : RS_DbsEntity() {}
    virtual ~RS_DbsLine() {}
    
    /*
    virtual int getTypeId() {
        return 2;
    }
    */
    
    static void registerType();

    virtual void initDb(RS_DbConnection& db);
    virtual RS_Entity* instantiate(RS_DbConnection& db, RS_Entity::Id entityId);
    virtual void save(RS_DbConnection& db, RS_Entity& entity);
    virtual void deleteEntity(RS_DbConnection& db, RS_Entity::Id entityId);
};
