drop view if exists vw_users;

create view vw_users as
select
  lower(m.username) as username,
  member_id + 10000 as uid,
  10000 as gid,
  '' as gecos,
  concat('/home/', lower(m.username)) as homedir,
  '/bin/bash' as shell,
  'x' as password,
  '1' as lstchg,
  '0' as min,
  '99999' as max,
  '0' as warn,
  '0' as inact,
  '-1' as expire,
  '0' as flag
from members m
where m.username is not null
  and m.member_status = 5;

