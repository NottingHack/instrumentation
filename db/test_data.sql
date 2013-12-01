insert into members (member_id, firstname, surname, email, join_date, unlock_text, balance, credit_limit, member_status, username, account_id, address_1, address_2, address_city, address_postcode, contact_number)
values (1, 'test' ,'member', 'test@example.org', sysdate(), 'Welcome test!', 0, 5000, 5, 'tmember', NULL, 'Unit F6 BizSpace','Roden House Business Centre', 'Nottingham', 'NG3 1JH', '0115 837 1505');


insert into rfid_tags (member_id, rfid_serial, state)
values (1, '1293205012', 10);

insert into pins (pin, state, member_id)
values ('1234', 40, 1);

insert into member_group (member_id, grp_id)
values (1, 1);

insert into products (product_id, price, available, shortdesc, longdesc)
values (1, 50, 1, 'Crisps', 'Packet of Crisps');
