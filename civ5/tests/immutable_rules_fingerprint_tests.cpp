#include "civ5/content/rules_database.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <iostream>
#include <stdexcept>
#include <thread>
#include <array>

using namespace civ5::content;
namespace {
int checks=0;
void check(bool ok,const char *name){++checks;if(!ok)throw std::runtime_error(name);}
template<class F> void rejects(F f,const char *name){bool failed=false;try{f();}catch(const std::exception&){failed=true;}check(failed,name);}
void setup(RulesDatabase &db){db.apply_sql("CREATE TABLE T(ID INTEGER,Type TEXT UNIQUE,V INTEGER);INSERT INTO T VALUES(0,'A',4),(1,'B',5);");}
void scalar(RulesDatabase &db,std::int64_t expected){db.query({"SELECT V FROM T WHERE Type='A'",{}},[&](auto row){check(std::get<std::int64_t>(row[0])==expected,"scalar data");});}
void poisoned(RulesDatabase &db){check(!db.immutable_preview_fingerprints_current(),"poison currentness");rejects([&]{(void)db.gameplay_fingerprint_data();},"poison canonical refusal");rejects([&]{(void)db.gameplay_fingerprint_sha256();},"poison digest refusal");rejects([&]{db.seal_immutable_preview_fingerprints();},"no reseal");}
}
int main(){try{
    {
        RulesDatabase db;setup(db);
        const auto bytes=db.gameplay_fingerprint_data();const auto sha=civ5::runtime::identify_bytes(bytes).sha256;
        db.seal_immutable_preview_fingerprints();
        check(db.immutable_preview_fingerprints_current(),"explicit current scope");
        for(int i=0;i<100;++i)check(db.gameplay_fingerprint_sha256()==sha,"exact cached digest");
        check(db.gameplay_fingerprint_data()==bytes,"exact canonical bytes");
        check(db.immutable_preview_fingerprint_serializations()==1,"one default serialization");
        rejects([&]{(void)db.gameplay_fingerprint_sha256({},bytes.size()-1);},"cache hit budget");
        check(db.immutable_preview_fingerprints_current(),"budget failure not writer");
        scalar(db,4);
        auto clone=db.clone_gameplay();clone->apply_sql("UPDATE T SET V=9");scalar(db,4);scalar(*clone,9);
    }
    {
        RulesDatabase db;setup(db);const auto before=db.gameplay_fingerprint_data();
        db.script_query("BEGIN",{})->step();(void)db.query_rows({"UPDATE T SET V=8",{}},{});
        rejects([&]{(void)db.gameplay_fingerprint_data();},"legacy unfinished transaction refusal");
        db.script_query("ROLLBACK",{})->step();check(db.gameplay_fingerprint_data()==before,"rollback old canonical");
        (void)db.query_rows({"CREATE TABLE Other(X)",{}},{});check(db.gameplay_fingerprint_data()!=before,"mutable DDL canonical");
        (void)db.query_rows({"DROP TABLE Other",{}},{});check(db.gameplay_fingerprint_data()==before,"schema restore canonical");
        const auto partial=db.apply_mod_sql("UPDATE T SET V=7; BAD SQL;");
        check(!partial.success&&partial.completed_statements==1,"partial mod failure semantics");scalar(db,7);
        db.apply_sql("CREATE TRIGGER rollback_write BEFORE UPDATE ON T BEGIN SELECT RAISE(ROLLBACK,'no'); END;");
        const auto old=db.gameplay_fingerprint_data();const auto rollback=db.apply_mod_sql("UPDATE T SET V=20;");
        check(!rollback.success,"trigger rollback error");check(db.gameplay_fingerprint_data()==old,"trigger rollback canonical");
    }
    {
        RulesDatabase db;setup(db);auto cursor=db.script_query("UPDATE T SET V=33 WHERE Type='A'",{});
        check(static_cast<bool>(cursor),"preprepared write");db.seal_immutable_preview_fingerprints();
        (void)db.gameplay_fingerprint_sha256();rejects([&]{cursor->step();},"preprepared write step");
        poisoned(db);cursor->reset();cursor->release();
    }
    {
        RulesDatabase db;setup(db);auto cursor=db.script_query("SELECT * FROM T",{});
        db.seal_immutable_preview_fingerprints();check(cursor->step(),"preprepared read");
        check(cursor->row().size()==3,"owned row");cursor->reset();check(cursor->step(),"read reset valid");cursor->release();
        check(db.immutable_preview_fingerprints_current(),"read retains scope");
    }
    for(const auto *sql: {"UPDATE T SET V=6","DELETE FROM T","INSERT INTO T VALUES(2,'C',6)",
                         "CREATE TABLE Other(X)","DROP TABLE T","ALTER TABLE T ADD COLUMN Y",
                         "CREATE TRIGGER tr AFTER INSERT ON T BEGIN UPDATE T SET V=0; END",
                         "BEGIN","SAVEPOINT x","PRAGMA user_version=4","ATTACH ':memory:' AS Extra"}){
        RulesDatabase db;setup(db);db.seal_immutable_preview_fingerprints();(void)db.gameplay_fingerprint_sha256();
        rejects([&]{db.query({sql,{}},{});},"sealed SQL write attempt");poisoned(db);
    }
    {
        RulesDatabase db;setup(db);db.script_query("BEGIN",{})->step();
        rejects([&]{db.seal_immutable_preview_fingerprints();},"transaction acquisition");
        db.script_query("ROLLBACK",{})->step();db.seal_immutable_preview_fingerprints();check(db.immutable_preview_fingerprints_current(),"acquire after rollback");
    }
    for(const auto *sql:{"SELECT * FROM T","UPDATE T SET V=10 RETURNING Type"}){
        RulesDatabase db;setup(db);auto cursor=db.script_query(sql,{});check(cursor->step(),"busy cursor");
        rejects([&]{db.seal_immutable_preview_fingerprints();},"busy acquisition");cursor->release();
        db.seal_immutable_preview_fingerprints();check(db.immutable_preview_fingerprints_current(),"acquire after finalize");
    }
    {
        RulesDatabase db;setup(db);bool observed=false;
        db.query({"SELECT * FROM T",{}},[&](auto){rejects([&]{db.seal_immutable_preview_fingerprints();},"callback seal acquisition");observed=true;});
        check(observed,"callback ran");db.seal_immutable_preview_fingerprints();
        rejects([&]{db.query({"SELECT * FROM T",{}},[&](auto){(void)db.query_rows({"UPDATE T SET V=8",{}},{});});},"callback writer reentry");poisoned(db);
    }
    {
        RulesDatabase db;setup(db);db.seal_immutable_preview_fingerprints();bool refused=false;
        std::thread foreign([&]{try{(void)db.gameplay_fingerprint_sha256();}catch(const std::exception&){refused=true;}});foreign.join();
        check(refused,"foreign thread digest");poisoned(db);
    }
    {
        RulesDatabase db;setup(db);db.seal_immutable_preview_fingerprints();bool refused=false;
        std::thread foreign([&]{try{db.query({"SELECT * FROM T",{}},{});}catch(const std::exception&){refused=true;}});foreign.join();
        check(refused,"foreign thread read");poisoned(db);
    }
    {
        std::unique_ptr<RulesScriptCursor> cursor;
        {RulesDatabase db;setup(db);cursor=db.script_query("UPDATE T SET V=8",{});db.seal_immutable_preview_fingerprints();}
        rejects([&]{cursor->step();},"same impl survives wrapper");cursor->release();
    }
    {
        RulesDatabase db;setup(db);std::string table="T",type="Type",id="ID";
        const std::array<RulesFingerprintNormalization,1> n{{{table,type,id}}};
        const auto canonical=db.gameplay_fingerprint_data(n);const auto sha=civ5::runtime::identify_bytes(canonical).sha256;
        const auto plain=db.gameplay_fingerprint_data();(void)db.query_rows({"UPDATE T SET ID=ID+10",{}},{});
        check(db.gameplay_fingerprint_data(n)==canonical,"generated ID normalization stable");check(db.gameplay_fingerprint_data()!=plain,"default ID identity changed");
        const auto default_sha=db.gameplay_fingerprint_sha256();db.seal_immutable_preview_fingerprints();
        check(db.gameplay_fingerprint_sha256(n)==sha,"normalized exact hash");table="Absent";type="Bad";id="Bad";
        const std::array<RulesFingerprintNormalization,1> original{{{"T","Type","ID"}}};
        check(db.gameplay_fingerprint_sha256(original)==sha,"copied normalized key retained");
        check(db.gameplay_fingerprint_sha256()==default_sha,"default distinct cache");
        check(db.immutable_preview_fingerprint_serializations()==2,"two canonical cache slots");
        rejects([&]{(void)db.gameplay_fingerprint_sha256(original,canonical.size()-1);},"normalized cached budget");
        const std::array<RulesFingerprintNormalization,1> wrong{{{"T","Missing","ID"}}};
        rejects([&]{(void)db.gameplay_fingerprint_sha256(wrong);},"invalid normalized key not cache hit");
        const std::array<RulesFingerprintNormalization,2> duplicate{{{"T","Type","ID"},{"T","Type","ID"}}};
        rejects([&]{(void)db.gameplay_fingerprint_sha256(duplicate);},"duplicate normalization");
        check(db.immutable_preview_fingerprints_current(),"normalization error retains scope");
        check(db.gameplay_fingerprint_sha256(n)==default_sha,"missing table normalization original behavior");
        check(db.gameplay_fingerprint_sha256(n)==default_sha,"extra key exact uncached result");
        check(db.immutable_preview_fingerprint_serializations()==4,"bounded normalized cache");
    }
    {
        auto saved=RulesDatabase::new_saved_values();rejects([&]{saved->seal_immutable_preview_fingerprints();},"multiconnection saved DB excluded");
    }
    {
        RulesDatabase db;setup(db);auto cursor=db.script_query("UPDATE T SET V=7 RETURNING Type",{});
        db.seal_immutable_preview_fingerprints();rejects([&]{cursor->step();},"preprepared returning writer");poisoned(db);
    }
    {
        RulesDatabase db;setup(db);db.seal_immutable_preview_fingerprints();
        check(!db.script_query("UPDATE T SET V=7",{}),"native-style invalid iterator on denied prepare");poisoned(db);
    }
    {
        RulesDatabase db;setup(db);db.seal_immutable_preview_fingerprints();
        const auto result=db.query_rows({"UPDATE T SET V=7",{}},{});
        check(!result.all_succeeded(),"caught writer preparation diagnostic");poisoned(db);
    }
    {
        RulesDatabase db;setup(db);db.seal_immutable_preview_fingerprints();
        rejects([&]{(void)db.apply_mod_sql("UPDATE T SET V=7; BAD SQL;");},"mod SQL acquisition refusal");poisoned(db);
    }
    {
        RulesDatabase db;setup(db);db.seal_immutable_preview_fingerprints();
        const auto sha=db.gameplay_fingerprint_sha256();bool observed=false;
        db.query({"SELECT * FROM T",{}},[&](auto){check(db.gameplay_fingerprint_sha256()==sha,"nested readonly identity");observed=true;});
        check(observed&&db.immutable_preview_fingerprint_serializations()==1,"nested cache needs no reserialization");
    }
    {
        RulesDatabase db;setup(db);db.apply_sql("CREATE TABLE Defines(Name TEXT,Value);INSERT INTO Defines VALUES('X',42);");
        check(std::get<std::int64_t>(db.global_define("X",GlobalDefineKind::integer).value)==42,"prepared define before seal");
        db.seal_immutable_preview_fingerprints();
        check(std::get<std::int64_t>(db.global_define("X",GlobalDefineKind::integer).value)==42,"reprepared define after seal");
        check(db.cache_info_types().report().all_succeeded(),"existing readonly cache remains valid");
        check(db.immutable_preview_fingerprints_current(),"existing readonly services retain scope");
    }
    {
        RulesDatabase db;setup(db);const auto expected=db.gameplay_fingerprint_sha256();db.seal_immutable_preview_fingerprints();
        const std::string large(65536,'x');const std::array<RulesFingerprintNormalization,1> n{{{large,"Type","ID"}}};
        check(db.gameplay_fingerprint_sha256(n)==expected,"large missing normalized key exact");
        check(db.gameplay_fingerprint_sha256(n)==expected,"large key not retained");
        check(db.immutable_preview_fingerprint_serializations()==2,"bounded retained key storage");
    }
    {
        std::function<bool()> retained;std::unique_ptr<RulesScriptCursor> writer;
        {RulesDatabase db;setup(db);check(!db.immutable_preview_fingerprint_currentness(),"mutable owner has no scope handle");
         writer=db.script_query("UPDATE T SET V=12",{});db.seal_immutable_preview_fingerprints();
         retained=db.immutable_preview_fingerprint_currentness();check(retained(),"scope handle initially current");}
        check(retained(),"same Impl retained after wrapper destruction");
        bool foreign_current=true;std::thread foreign([&]{foreign_current=retained();});foreign.join();
        check(!foreign_current,"retained handle foreign thread refusal");
        rejects([&]{writer->step();},"retained Impl writer refusal");
        check(!retained(),"retained currentness poisoned without wrapper UAF");
        writer->release();writer.reset();check(!retained(),"retained poison remains after cursor destruction");
        retained={};
    }
    {
        RulesDatabase db,text;setup(db);text.apply_sql("CREATE TABLE Labels(Tag TEXT,Text TEXT);INSERT INTO Labels VALUES('K','Original');");
        db.attach_localization(text);const auto canonical=db.gameplay_fingerprint_data();
        db.seal_immutable_preview_fingerprints();auto clone=db.clone_gameplay();
        check(clone->query_rows({"SELECT Text FROM Localization.Labels",{}},[](const auto &row){check(row.text("Text")=="Original","attached selected source clone");}).all_succeeded(),"attached read clone");
        check(clone->apply_mod_sql("UPDATE Localization.Labels SET Text='Clone';").success,"clone independently mutable");
        check(db.query_rows({"SELECT Text FROM Localization.Labels",{}},[](const auto &row){check(row.text("Text")=="Original","authoritative text unchanged");}).all_succeeded(),"sealed attached source readable");
        check(db.gameplay_fingerprint_data()==canonical,"old main-only fingerprint scope unchanged");
        rejects([&]{db.attach_localization(text);},"sealed attachment attempt");poisoned(db);
    }
    std::cout<<"PASS "<<checks<<" checks; exact canonical/SHA, mutable semantics, one-way scope, writers/thread/reentry/lifetime\n";
    return 0;
}catch(const std::exception &e){std::cerr<<"FAIL "<<checks<<" "<<e.what()<<'\n';return 1;}}
