drop procedure if exists sp_web_group_add_member;


DELIMITER //
CREATE PROCEDURE sp_web_group_add_member
(
   IN  grp_id     int,
   IN  member_id  int,
   OUT err        varchar(100)
)
SQL SECURITY DEFINER
BEGIN


  main: begin
    set err = '';
  
    replace into member_group (member_id, grp_id)
    values (member_id, grp_id);
    
  end main;
  

END //
DELIMITER ;
