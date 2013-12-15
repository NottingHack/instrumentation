drop procedure if exists sp_web_group_add;


DELIMITER //
CREATE PROCEDURE sp_web_group_add
(
   IN  grp_description  varchar(200),
   OUT err              varchar(100)
)
SQL SECURITY DEFINER
BEGIN
  declare group_exists int;
  set group_exists = 0;
  set err = '';

  select count(*) 
  into group_exists
  from grp g
  where g.grp_description = grp_description;
  
  if (group_exists = 0) then
    insert into grp (grp_description)
    values (grp_description);
  else
    set err = 'Group already exists';
  end if;  

END //
DELIMITER ;

