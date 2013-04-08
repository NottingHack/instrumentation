drop procedure if exists sp_web_group_delete_member;


DELIMITER //
CREATE PROCEDURE sp_web_group_delete_member
(
   IN  grp_id     int,
   IN  member_id  int,
   OUT err        varchar(100)
)
SQL SECURITY DEFINER
BEGIN


  main: begin
    set err = '';
  
    delete from member_group
    where member_group.member_id = member_id
    and member_group.grp_id = grp_id;
    
  end main;
  

END //
DELIMITER ;
