drop procedure if exists sp_web_group_toggle_permission;


DELIMITER //
CREATE PROCEDURE sp_web_group_toggle_permission
(
   IN  grp_id           int,
   IN  permission_code  varchar(16),
   OUT err              varchar(100)
)
SQL SECURITY DEFINER
BEGIN
  declare permission_enabled int;
  set permission_enabled = 0;
  set err = '';

  select count(*) 
  into permission_enabled
  from group_permissions gp
  where gp.grp_id = grp_id
  and gp.permission_code = permission_code;
  
  if (permission_enabled > 0) then
    delete from group_permissions
    where group_permissions.grp_id = grp_id
    and group_permissions.permission_code = permission_code;    
  else
    insert into group_permissions (grp_id, permission_code)
    values (grp_id, permission_code);
  end if;


END //
DELIMITER ;
