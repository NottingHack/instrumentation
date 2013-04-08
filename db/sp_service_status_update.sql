drop procedure if exists sp_service_status_update;

DELIMITER //
CREATE PROCEDURE sp_service_status_update
(
   IN timeout_period int
)
SQL SECURITY DEFINER
BEGIN
  
  update service_status
  set status = 0, status_str=null
  where reply_time is null
     or TIMESTAMPDIFF(SECOND,reply_time, query_time) >= timeout_period;

END //
DELIMITER ;
