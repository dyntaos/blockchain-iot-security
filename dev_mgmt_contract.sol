pragma solidity >=0.6.0;

//TODO: Can all functions be non-payable? How to handle ether? SOLUTION: On miners: "geth ... --gasprice 0 ..." https://stackoverflow.com/questions/49318479/how-to-make-sure-transactions-take-0-fee-in-a-private-ethereum-blockchain
//TODO?: Implement log of events?
//TODO?: May not be needed: 0 Cannot be a region ID -- If the region is not active, then just ignore this value, for if it is active it will surely be in activeMangementRegions
//TODO: Link redgion indexes in devices to mgmt Regions
//TODO: Link authorizedAdminUsersIndex to active_admin_users
//TODO: Deactivate region/reactivate region & delete region (Consider implications of Device.mangementRegionIndexes)
//TODO: Initial admin user
//TODO: Function to change region of device
//TODO: getters for Regions
//TODO: getters for all structs


/*
 *
 */
contract DeviceMgmt {

	enum AddrType {NULL, IP, LORA, ZIGBEE, BLUETOOTH, OTHER} //TODO: Add other common protocols and custom values


	/*
	 *
	 */
	struct Device {
		address eth_addr;
		bool has_eth_addr;
		string name;                            // Arbitrary, admin assigned, human readable name
		bool active;
		bool is_gateway;                        //
		uint32 device_id;                       // Integer ID of this device (NEW)
		uint creationTimestamp;                 // Time this device was first created
		uint dataTimestamp;                     // NEW
		string data;                            // Data can point to swarm address or just contain raw IoT device data such as sensor information (encrypted)

		AddrType addrType;                      // How the device connects to the network (IP (ethernet, WiFi, GSM/LTE), LoRa, etc.)
		string addr;                            // Device is to maintain its current IP (v4/v6) address
		string mac;                             // MAC address for further identification

		string publicKey;                       // Devices public encryption key
		bool gateway_managed;                   // Is this deviced managed by the set of gateway accounts  (NEW)
		uint32 authorizedDevicesIndex;          // TODO: Remove?
	}


	/*
	 *
	 */
	struct AdminUser {
		bool isAdmin;                           //
		uint32 authorizedAdminUsersIndex;       // The index within active_admin_users which points to this AdminUser
	}

	uint32 next_device_id;
	uint32[] free_device_id_stack;
	mapping (address => uint32) addr_to_id;
	mapping (uint32 => Device) id_to_device;  // device_id => Device

	mapping (address => bool) gateway_pool;

	mapping (address => AdminUser) admin_mapping;
	address[] active_admin_users;

	address[] authorized_devices;



	// TODO: Should we index any of these event topics?
	event Add_Device(address clientAddr, string name, string mac, string publicKey, bool gateway_managed, uint32 device_id);
	event Add_Gateway(address clientAddr, string name, string mac, string publicKey, uint32 device_id);
	event Remove_Device(uint32 device_id);
	event Remove_Gateway(address gateway_addr, uint32 device_id);
	event Push_Data(uint32 device_id, uint timestamp, string data);
	event Update_Addr(uint32 device_id, uint addrType, string addr);
	event Authorize_Admin(address newAdminAddr);
	event Deauthorize_Admin(address adminAddr);



	/*
	 * @dev
	 */
	modifier _authorized {
		require(id_to_device[addr_to_id[msg.sender]].active || admin_mapping[msg.sender].isAdmin);
		_;
	}


	/*
	 * @dev
	 */
	modifier _authorizedDeviceOnly {
		require(id_to_device[addr_to_id[msg.sender]].active);
		_;
	}


	/*
	 * @dev
	 */
	modifier _authorizedDeviceOrGateway {
		require(id_to_device[addr_to_id[msg.sender]].active || gateway_pool[msg.sender]);
		_;
	}


	/*
	 * @dev
	 */
	modifier _gateway {
		require(gateway_pool[msg.sender]);
		_;
	}


	/*
	 * @dev
	 */
	modifier _mutator(uint32 device_id) {
		require(id_to_device[device_id].active);
		if (gateway_pool[msg.sender]) {
			require(id_to_device[device_id].gateway_managed || id_to_device[device_id].eth_addr == msg.sender);
		} else {
			require(id_to_device[device_id].eth_addr == msg.sender);
		}
		_;
	}


	/*
	 * @dev
	 */
	modifier _admin {
		require(admin_mapping[msg.sender].isAdmin);
		_;
	}


	/*
	 * @dev
	 */
	constructor() public {
		next_device_id = 0;
		admin_mapping[msg.sender].isAdmin = true;
		active_admin_users.push(msg.sender);
		admin_mapping[msg.sender].authorizedAdminUsersIndex = uint32(active_admin_users.length - 1);
	}


	//TODO: Which of fallback and/or receive is needed?
	//fallback() external payable {}

	//TODO: ETHABI fails to encode calls when this exists as it currently stands in ABI form
	//receive() external payable {}


	 /*
	  * @dev Is the given address an admin? Only for external calls. For internal use, test: admin_mapping[clientAddr].isAdmin
	  * @param clientAddr
	  * @return
	  */
	function is_admin(address clientAddr) external view _authorized returns(bool) {
		return admin_mapping[clientAddr].isAdmin;
	}


	/*
	 * @dev Is the given address an authorized device? Only for external calls. For internal use, test: id_to_device[clientAddr].active
	 * @param
	 * @return
	 */
	function is_authd(uint32 device_id) external view _authorized returns(bool) {
		return id_to_device[device_id].active;
	}


	/*
	 * @dev
	 * @param
	 * @return
	 */
	function get_my_device_id() external view returns(int64) {
		if (id_to_device[addr_to_id[msg.sender]].active) {
			return addr_to_id[msg.sender];
		} else {
			return -1;
		}
	}


	/*
	 * @dev
	 * @param
	 * @return
	 */
	function get_key(uint32 device_id) external view _authorized returns(string memory) {
		require(id_to_device[device_id].active);
		return id_to_device[device_id].publicKey;
	}


	/*
	 * @dev
	 * @param
	 * @return
	 */
	function get_addrtype(uint32 device_id) external view _authorized returns(uint) {
		require(id_to_device[device_id].active);
		return uint(id_to_device[device_id].addrType);
	}


	/*
	 * @dev
	 * @param
	 * @return
	 */
	function get_addr(uint32 device_id) external view _authorized returns(string memory) {
		require(id_to_device[device_id].active);
		return id_to_device[device_id].addr;
	}


	/*
	 * @dev
	 * @param
	 * @return
	 */
	function get_mac(uint32 device_id) external view _authorized returns(string memory) {
		require(id_to_device[device_id].active);
		return id_to_device[device_id].mac;
	}


	// TODO: Modifier should probably be _authorizedDeviceOrGateway
	/*
	 * @dev
	 * @param
	 * @return
	 */
	function get_data(uint32 device_id) external view _authorized returns(string memory) {
		require(id_to_device[device_id].active);
		return id_to_device[device_id].data;
	}


	/*
	 * @dev
	 * @param clientAddr
	 * @param name
	 * @param mac
	 * @param publicKey
	 * @param gateway_managed
	 * @return
	 */
	function add_device(address clientAddr, string calldata name, string calldata mac, string calldata publicKey, bool gateway_managed) external _admin returns(uint32) {
		require(gateway_managed || (!id_to_device[addr_to_id[clientAddr]].active));
		uint32 device_id;
		if (free_device_id_stack.length > 0) {
			device_id = free_device_id_stack[free_device_id_stack.length - 1];
			free_device_id_stack.pop(); //TODO: What the hell does this return!?
		} else {
			device_id = next_device_id++; //TODO: What to do if this overflows.
		}

		id_to_device[device_id].name = name;
		id_to_device[device_id].active = true;
		id_to_device[device_id].is_gateway = false;
		id_to_device[device_id].addrType = AddrType.NULL;
		id_to_device[device_id].addr = "";
		id_to_device[device_id].mac = mac;
		id_to_device[device_id].data = "";
		id_to_device[device_id].dataTimestamp = 0;
		id_to_device[device_id].publicKey = publicKey;
		id_to_device[device_id].creationTimestamp = now;
		id_to_device[device_id].gateway_managed = gateway_managed;

		if (gateway_managed) {
			id_to_device[device_id].eth_addr = clientAddr;
			id_to_device[device_id].has_eth_addr = true;
		} else {
			addr_to_id[clientAddr] = id_to_device[device_id].device_id;
			id_to_device[device_id].has_eth_addr = false;
		}

		emit Add_Device(clientAddr, name, mac, publicKey, gateway_managed, device_id);
		return device_id;
	}


	/*
	 * @dev
	 * @param clientAddr
	 * @param name
	 * @param mac
	 * @param publicKey
	 * @return
	 */
	function add_gateway(address clientAddr, string calldata name, string calldata mac, string calldata publicKey) external _admin returns(uint32) {
		require(!id_to_device[addr_to_id[clientAddr]].active);
		uint32 device_id;
		if (free_device_id_stack.length > 0) {
			device_id = free_device_id_stack[free_device_id_stack.length - 1];
			free_device_id_stack.pop();
		} else {
			device_id = next_device_id++; //TODO: What to do if this overflows.
		}

		id_to_device[device_id].name = name;
		id_to_device[device_id].active = true;
		id_to_device[device_id].is_gateway = true;
		id_to_device[device_id].addrType = AddrType.NULL;
		id_to_device[device_id].addr = "";
		id_to_device[device_id].mac = mac;
		id_to_device[device_id].data = "";
		id_to_device[device_id].dataTimestamp = 0;
		id_to_device[device_id].publicKey = publicKey;
		id_to_device[device_id].creationTimestamp = now;
		id_to_device[device_id].gateway_managed = false;
		id_to_device[device_id].eth_addr = clientAddr;
		id_to_device[device_id].has_eth_addr = true;

		gateway_pool[clientAddr] = true;
		addr_to_id[clientAddr] = id_to_device[device_id].device_id;

		emit Add_Gateway(clientAddr, name, mac, publicKey, device_id);
		return device_id;
	}


	/*
	 * @dev
	 * @param clientAddr
	 * @return
	 */
	function remove_device(uint32 device_id) external _admin returns(bool) {
		require(id_to_device[device_id].active);

		free_device_id_stack.push(device_id);
		if (id_to_device[device_id].has_eth_addr) {
			delete addr_to_id[id_to_device[device_id].eth_addr];
		}
		delete id_to_device[device_id];

		emit Remove_Device(device_id);
		return true;
	}


	/*
	 * @dev
	 * @param
	 * @return
	 */
	function remove_gateway(uint32 device_id) external _admin returns(bool) {
		address gateway_addr = id_to_device[device_id].eth_addr;
		require(gateway_pool[gateway_addr]);

		free_device_id_stack.push(device_id);
		delete gateway_pool[gateway_addr];
		delete id_to_device[device_id];
		delete addr_to_id[gateway_addr];

		emit Remove_Gateway(gateway_addr, device_id);
		return true;
	}


	/*
	 * @dev
	 * @param addrType
	 * @param addr
	 * @return
	 */
	function update_addr(uint32 device_id, uint addrType, string calldata addr) external _authorizedDeviceOrGateway _mutator(device_id) returns(bool) {
		id_to_device[device_id].addrType = AddrType(addrType);
		id_to_device[device_id].addr = addr;

		emit Update_Addr(device_id, addrType, addr);
		return true;
	}


	/*
	 * @dev
	 * @param addrType
	 * @param addr
	 * @return
	 */
	function push_data(uint32 device_id, string calldata data) external _authorizedDeviceOrGateway _mutator(device_id) returns(bool) {
		id_to_device[device_id].data = data;
		id_to_device[device_id].dataTimestamp = now;

		emit Push_Data(device_id, now, data); //TODO: Data will be stored as a log AND data? Is just a log sufficient?
		return true;
	}


	/*
	 * @dev
	 * @param newAdminAddr
	 * @return
	 */
	function authorize_admin(address newAdminAddr) external _admin returns(bool) {
		admin_mapping[newAdminAddr].isAdmin = true;
		active_admin_users.push(newAdminAddr);
		admin_mapping[newAdminAddr].authorizedAdminUsersIndex = uint32(active_admin_users.length - 1);

		emit Authorize_Admin(newAdminAddr);
		return true;
	}


	/*
	 * @dev
	 * @param adminAddr
	 * @return
	 */
	function deauthorize_admin(address adminAddr) external _admin returns(bool) {
		require(active_admin_users.length > 1);

		if (active_admin_users.length - 1 == admin_mapping[adminAddr].authorizedAdminUsersIndex) {
			// If the index of the admin user is the last item in authorizedAdminUsers, just delete it
			delete active_admin_users[admin_mapping[adminAddr].authorizedAdminUsersIndex];
		} else {
			// If it is not the last item, swap the last item to this index and delete the last item (keep array from fragmenting)
			//TODO: REVIEW THIS LOGIC
			active_admin_users[admin_mapping[adminAddr].authorizedAdminUsersIndex] = active_admin_users[active_admin_users.length - 1];
			admin_mapping[active_admin_users[active_admin_users.length - 1]].authorizedAdminUsersIndex = admin_mapping[adminAddr].authorizedAdminUsersIndex;
			delete active_admin_users[active_admin_users.length - 1];
		}

		admin_mapping[adminAddr].authorizedAdminUsersIndex = 0;
		admin_mapping[adminAddr].isAdmin = false;

		emit Deauthorize_Admin(adminAddr);
		return true;
	}


	/*
	 * @dev
	 * @return
	 */
	function get_num_admin() external view _admin returns(uint16) {
		return uint16(active_admin_users.length);
	}


	/*
	 * @dev
	 * @return
	 */
	 function get_active_admins() public view _admin returns(address[] memory) {
		 return active_admin_users;
	}


	/*
	 * @dev
	 * @return
	 */
	function get_authorized_devices() public view _admin returns(address[] memory) {
		return authorized_devices;
	}

}
