drop procedure if exists sp_gatekeeper_get_doors;

/*
  Get details of all doors
*/

DELIMITER //
CREATE PROCEDURE sp_gatekeeper_get_doors
(
   IN  p_door_id int
)
SQL SECURITY DEFINER
BEGIN
  main: begin

  select
    d.door_id,
    d.door_description,
    d.door_short_name,
    d.door_state,
    d.door_state
  from doors d
  where (d.door_id = p_door_id or p_door_id = -1);

  end main;
 

END //
DELIMITER ;
