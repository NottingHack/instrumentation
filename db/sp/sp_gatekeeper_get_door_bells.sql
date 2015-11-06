drop procedure if exists sp_gatekeeper_get_door_bells;

/*
  Get details of all bells that should be rang with the door bell button for door_id is pushed
*/

DELIMITER //
CREATE PROCEDURE sp_gatekeeper_get_door_bells
(
   IN  p_door_id int
)
SQL SECURITY DEFINER
BEGIN
  main: begin

    select distinct
      b.bell_topic,
      b.bell_message
    from doors d 
    inner join door_bells db on db.door_id = d.door_id
    inner join bells b on b.bell_id = db.bell_id
    where b.bell_enabled = 1
      and d.door_id = p_door_id;

  end main;
 

END //
DELIMITER ;
