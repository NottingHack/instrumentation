drop procedure if exists sp_record_service_status_request;

DELIMITER //
CREATE PROCEDURE sp_record_service_status_request
(
   IN s_service_name  varchar(256)
)
SQL SECURITY DEFINER
BEGIN
    update service_status
    set query_time = sysdate();
END //
DELIMITER ;

