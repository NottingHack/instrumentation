
drop procedure if exists sp_tool_delete_google_channel;

/*
  Delete from the tl_google_notifications table. 

*/

DELIMITER //
CREATE PROCEDURE sp_tool_delete_google_channel
(
  IN p_channel_id    varchar(256)
)
SQL SECURITY DEFINER
BEGIN

  main: begin
  
    delete from tl_google_notifications
    where channel_id = p_channel_id;
    
  end main;

END //
DELIMITER ;
