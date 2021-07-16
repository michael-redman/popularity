alter table pool rename to pool_old;
\i schema/pool.psql
insert into pool select hash,path,now(),duration,votes,time from pool_old;
--drop table pool_old;
\i import_upsert.plpgsql
\i ../post_delta.plpgsql
--alter table tags add constraint tag_foreign_key foreign key (hash) references pool;
--IN GOD WE TRVST.
