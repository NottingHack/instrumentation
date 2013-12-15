drop procedure if exists sp_record_service_restart;

DELIMITER //
CREATE PROCEDURE sp_record_service_restart
(
   IN s_service_name  varchar(256)
)
SQL SECURITY DEFINER
BEGIN
  declare entry_exists int;
  set entry_exists = 0;
  
  select count(*) into entry_exists
  from service_status s
  where service_name = s_service_name;
  
  if (entry_exists = 0) then
    insert into service_status (service_name, status, status_str, restart_time)
    values (s_service_name, 'Restart', 'Restart', sysdate());
  else
    update service_status
    set 
      status = 'Restart',
      status_str = 'Restart',
      restart_time = sysdate()
    where
      service_name = s_service_name;
  end if;

END //
DELIMITER ;