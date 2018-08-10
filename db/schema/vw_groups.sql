drop view if exists vw_groups;

create view vw_groups as
select
  'members' as name,
  'x' as password,
  '10000' as gid;
