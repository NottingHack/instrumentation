
-- Create "Full access" user
insert into members (member_id, firstname, surname, email, join_date, unlock_text, balance, credit_limit, member_status, username, account_id, address_1, address_2, address_city, address_postcode, contact_number)
values (1, 'test' ,'admin', 'admin@example.org', sysdate(), 'Welcome admin', 0, 2000, 5, 'admin', 1, 'Unit F6 BizSpace','Roden House Business Centre', 'Nottingham', 'NG3 1JH', '0115 837 1505');

insert into member_group (member_id, grp_id)
values (1, 1);

insert into account (payment_ref, account_id)
values ('ADMINREF', 1);

insert into rfid_tags (member_id, rfid_serial, state)
values (1, '883059226', 10);

insert into pins (pin, state, member_id)
values ('1234', 40, 1);

-- Create "normal" user
insert into members (member_id, firstname, surname, email, join_date, unlock_text, balance, credit_limit, member_status, username, account_id, address_1, address_2, address_city, address_postcode, contact_number)
values (2, 'test' ,'test', 'test@example.org', sysdate(), 'Welcome test!', 0, 2000, 5, 'test', 2, 'Unit F6 BizSpace','Roden House Business Centre', 'Nottingham', 'NG3 1JH', '0115 837 1505');

insert into account (payment_ref, account_id)
values ('TESTREF', 2);

-- add a product
insert into products (product_id, price, available, shortdesc, longdesc)
values (1, 50, 1, 'Crisps', 'Packet of Crisps');

insert into vmc_state (vmc_ref_id, product_id)
values (1,1); -- set vending machine location A1 to be a "Packet of Crisps"

-- Record a purchase
call sp_transaction_log(2, 50, 'VEND', 'COMPLETE', '[Packet of Crisps] vended from location [A1]', NULL, @tran_id, @err);

-- Record a payment
call sp_transaction_log(2, -50, 'MANUAL', 'COMPLETE', 'Payment', 1, @tran_id, @err);

-- Tools
insert into tl_tools values (1, '', 'laser', 'FREE', 'RESTRICTED', null, 300, 10, 10, NULL, '', 0);


-- Usage - add 2mins of pledged time
insert into tl_tool_usages values (1, 1, 1, sysdate(), -120, 0, 'COMPLETE');
-- add 30 seconds of use
insert into tl_tool_usages values (2, 1, 1, sysdate(), 30, 0, 'COMPLETE');
-- all member_id=1 to use tool_id=1
insert into tl_members_tools values (1, 1, 1, 1, sysdate(), 'USER');
