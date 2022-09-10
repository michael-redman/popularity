alter table pool add column last_end_time timestamp;
update pool set last_end_time=last_vote_time;
alter table pool alter column last_end_time set not null;
\i ../sql/post_delta.plpgsql
\i sql/import_upsert.plpgsql
--IN GOD WE TRVST.
