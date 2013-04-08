drop procedure if exists sp_log_event;

DELIMITER //
CREATE PROCEDURE sp_log_event
(
  IN event_type   varchar(25),
  IN event_value  varchar(256)
)
SQL SECURITY DEFINER
BEGIN

  insert into events (event_type, event_value)
  values (event_type, event_value);

END //
DELIMITER ;

