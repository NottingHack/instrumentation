
drop procedure if exists sp_tool_set_google_channel;

/*
  Add an entry to the tl_google_notifications table. Either updates existing record,
  or creates a new one if a record with the same channel_id doesn't exist.
  p_expiration is expected to be a Unix timestamp.

*/

DELIMITER //
CREATE PROCEDURE sp_tool_set_google_channel
(
  IN p_tool_id       int,
  IN p_resource_id   varchar(256),
  IN p_channel_token varchar(256),
  IN p_channel_id    varchar(256),
  IN p_expiration    int
)
SQL SECURITY DEFINER
BEGIN

  main: begin
  
    declare l_expiration timestamp;
    set l_expiration = FROM_UNIXTIME(p_expiration);
    

    insert into tl_google_notifications (  channel_id,   tool_id,   channel_token,   resource_id, channel_expiration)
                                 values (p_channel_id, p_tool_id, p_channel_token, p_resource_id, l_expiration)
                                 on duplicate key update
                                 tool_id           = values(tool_id),
                                 channel_token     = values(channel_token),
                                 resource_id       = values(resource_id),
                                 channel_expiration= values(channel_expiration);
  end main;

END //
DELIMITER ;
