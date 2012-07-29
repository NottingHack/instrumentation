drop table if exists permissions;

create table permissions 
(
  permission_code varchar(16) not null,
  permission_desc varchar(200) not null,
  primary key (permission_code)
) ENGINE = InnoDB; 


/*insert into permissions (permission_code, permission_desc) values ('                ', '')*/

insert into permissions (permission_code, permission_desc) values ('VIEW_VEND_CONFIG', 'View vending machine setup (product in each location)');
insert into permissions (permission_code, permission_desc) values ('WEB_LOGON       ', 'Allow logon to nh-web');
insert into permissions (permission_code, permission_desc) values ('VIEW_PRODUCTS   ', 'View products');
insert into permissions (permission_code, permission_desc) values ('VIEW_PRD_DETAIL ', 'View product details');
insert into permissions (permission_code, permission_desc) values ('UPD_VEND_CONFIG ', 'Update vending machine config');
insert into permissions (permission_code, permission_desc) values ('VIEW_VEND_LOG   ', 'View vending machine log');
insert into permissions (permission_code, permission_desc) values ('VIEW_BALANCES   ', 'View member balances / credit limit');
insert into permissions (permission_code, permission_desc) values ('VIEW_TRANS      ', 'View member transactions');
insert into permissions (permission_code, permission_desc) values ('VIEW_OWN_TRANS  ', 'View own transactions');
insert into permissions (permission_code, permission_desc) values ('SET_CREDIT_LIMIT', 'Set member credit limit');
insert into permissions (permission_code, permission_desc) values ('REC_TRAN        ', 'Record transaction (against any member)');
insert into permissions (permission_code, permission_desc) values ('REC_TRAN_OWN    ', 'Record transaction (against self)');
insert into permissions (permission_code, permission_desc) values ('VIEW_GROUPS     ', 'View list of access groups');
insert into permissions (permission_code, permission_desc) values ('VIEW_GRP_MEMBERS', 'View group members');
insert into permissions (permission_code, permission_desc) values ('VIEW_MEMBERS    ', 'View members list (add member to group listbox - handle+id only)');
insert into permissions (permission_code, permission_desc) values ('REM_GRP_MEMBER  ', 'Remove member from group');
insert into permissions (permission_code, permission_desc) values ('ADD_GRP_MEMBER  ', 'Add member to group');
insert into permissions (permission_code, permission_desc) values ('CHG_GRP_PERM    ', 'Change/toggle state of group permissions');
insert into permissions (permission_code, permission_desc) values ('ADD_GROUP       ', 'Add group');
insert into permissions (permission_code, permission_desc) values ('DEL_GROUP       ', 'Delete group');
insert into permissions (permission_code, permission_desc) values ('VIEW_GRP_PERMIS ', 'View group permissions');
insert into permissions (permission_code, permission_desc) values ('ADD_UPD_PRODUCT ', 'Add / update product');
insert into permissions (permission_code, permission_desc) values ('VIEW_ACCESS_MEM ', 'View Access > Members');
insert into permissions (permission_code, permission_desc) values ('SET_PASSWORD    ', 'Set any members password');

 

 

/*

select *
from members m
inner join member_group mg on mg.member_id = m.member_id
inner join grp g on g.grp_id = mg.grp_id
inner join group_permissions gp on gp.grp_id = g.grp_id
inner join permissions p on p.permission_code = gp.permission_code;


        if (!check_permission($_SESSION['member_id'], "REC_TRAN"))
        { 
          $error->SetError(JsonRpcError_PermissionDenied, "Permission Denied (REC_TRAN)");
          return $error;          
        }

*/


 