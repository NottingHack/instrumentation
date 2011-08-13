drop procedure if exists sp_record_service_status_request;

DELIMITER //
CREATE PROCEDURE sp_record_service_status_request
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
    insert into service_status (service_name, query_time)
    values (s_service_name, sysdate());
  else
    update service_status
    set query_time = sysdate()
    where service_name = s_service_name;
  end if;

END //
DELIMITER ;


GRANT EXECUTE ON PROCEDURE sp_record_service_status_request TO 'gk'@'localhost'