drop procedure if exists sp_add_card;
/* err is null on success */
DELIMITER //
CREATE PROCEDURE sp_add_card
(
   IN member_id   int,
   IN rfid_serial varchar(50),
   OUT err        varchar(100)
)
SQL SECURITY DEFINER
BEGIN
  declare card_exists int;
  declare member_exists int;
  set card_exists = 1;
  set member_exists = 0;
  
  select count(*) into member_exists
  from members m
  where m.member_id = member_id;
  
  if (member_exists != 1) then
    set err = "Member not found";
   else
    
    select count(*) into card_exists
    from rfid_tags t
    where t.rfid_serial = rfid_serial
      and state = 10; -- active
    
    if (card_exists = 0) then
      insert into rfid_tags (member_id, rfid_serial, state) values (member_id, rfid_serial, 10);
      set err = null;
    else
      set err = "Active card with same serial already in database";
    end if;
  end if;

END //
DELIMITER ;
