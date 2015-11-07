drop procedure if exists sp_set_door_state;
/*

*/

DELIMITER //
CREATE PROCEDURE sp_set_door_state 
(
   IN   p_door_id     int,
   IN   p_door_state  varchar(10)
)
SQL SECURITY DEFINER
BEGIN

  main: begin
  
    declare evt varchar(100);
    
    set evt = case 
        when p_door_state = 'OPEN'   then 'DOOR_OPENED' 
        when p_door_state = 'CLOSED' then 'DOOR_CLOSED'
        else 'UNKNOWN'
      end;
    
    update doors
    set 
      door_state        = p_door_state,
      door_state_change = sysdate()
    where door_id = p_door_id;

  call sp_log_event(evt, p_door_id);

  end main;


END //
DELIMITER ;

