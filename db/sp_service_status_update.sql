drop procedure if exists sp_service_status_update;

DELIMITER //
CREATE PROCEDURE sp_service_status_update
(
   IN timeout_period int
)
SQL SECURITY DEFINER
BEGIN
  
  update service_status
  set 
    status = 0,
    status_str = case 
      when status_str='Running' then ''
      else status_str -- If the status text is something other than "Running" (E.g. "terminated"), then keep it
    end
  where 
  (
    reply_time is null or
    TIMESTAMPDIFF(SECOND,reply_time, query_time) >= timeout_period
  ) and service_name != 'Mosquitto';

END //
DELIMITER ;
