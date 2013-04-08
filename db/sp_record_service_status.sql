drop procedure if exists sp_record_service_status;

DELIMITER //
CREATE PROCEDURE sp_record_service_status
(
   IN s_service_name  varchar(256),
   IN s_status        int,
   IN s_status_str    varchar(1024)
)
SQL SECURITY DEFINER
BEGIN
  declare entry_exists int;
  set entry_exists = 0;
  
  select count(*) into entry_exists
  from service_status s
  where service_name = s_service_name;
  
  if (entry_exists = 0) then
    insert into service_status (service_name, status, status_str, reply_time)
    values (s_service_name, s_status, s_status_str, sysdate());
  else
    update service_status
    set 
      status = s_status,
      status_str = s_status_str,
      reply_time = sysdate()
    where
      service_name = s_service_name;
  end if;

END //
DELIMITER ;
