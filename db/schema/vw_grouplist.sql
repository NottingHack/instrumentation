drop view if exists vw_grouplist;

create view vw_grouplist as
select
  -- rowid,
  '10000' as gid,
  lower(m.username) as username
from members m
where m.username is not null
  and m.member_status = 5;
