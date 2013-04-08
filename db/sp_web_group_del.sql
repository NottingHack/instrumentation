drop procedure if exists sp_web_group_del;


DELIMITER //
CREATE PROCEDURE sp_web_group_del
(
   IN  grp_id int,
   OUT err    varchar(100)
)
SQL SECURITY DEFINER
BEGIN
  declare group_empty int;
  set group_empty = 0;
  set err = '';

  select count(*) 
  into group_empty
  from member_group mg
  where mg.grp_id = grp_id;
  
  if (group_empty = 0) then
    delete from grp
    where grp.grp_id = grp_id;
    
    delete from group_permissions
    where group_permissions.grp_id = grp_id;
  else
    set err = 'Group is not empty';
  end if;  

END //
DELIMITER ;

