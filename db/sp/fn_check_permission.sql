drop function if exists fn_check_permission;

/*
  Check if member_id has been assigned permission_code.
  Returns 1 if yes, 0 if no
 */


DELIMITER //
CREATE FUNCTION fn_check_permission
(
    member_id int, 
    permission_desc varchar(200) 
)
returns int
DETERMINISTIC
READS SQL DATA
SQL SECURITY DEFINER
BEGIN
  declare c int;
  set c = 0;

  select count(*)
  into c
  from members m
  inner join member_group mg on mg.member_id = m.member_id
  inner join grp g on g.grp_id = mg.grp_id
  inner join group_permissions gp on gp.grp_id = g.grp_id
  inner join permissions p on p.permission_code = gp.permission_code
  where m.member_id = member_id
  and p.permission_code = permission_desc;

  if (c >= 1) then
    set c = 1;
  else
    set c = 0;
  end if;
  
  return c;
END //
DELIMITER ;


 