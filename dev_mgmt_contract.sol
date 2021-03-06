pragma solidity >=0.6.0;



/*
 *
 */
contract DeviceMgmt {

	enum AddrType {UNSET, IPV4, IPV6, LORA}

	/*
	 *
	 */
	struct Device {
		address eth_addr;
		bool has_eth_addr;
		string name;                            // Arbitrary, admin assigned, human readable name
		bool active;
		bool is_gateway;                        //
		uint32 device_id;                       // Integer ID of this device
		uint32 creationTimestamp;               // Time this device was first created
		uint32 dataTimestamp;                   //

		string data;                            // Data can point to swarm address or just contain raw IoT device data such as sensor information (encrypted)
		uint16 dataLen;
		string nonce;

		AddrType addrType;                      // How the device connects to the network (IP (ethernet, WiFi, GSM/LTE), LoRa, etc.)
		string addr;                            // Device is to maintain its current IP (v4/v6) address

		string publicKey;                       // Devices public encryption key
		string signPublicKey;                   //
		uint32 dataReceiverID;                  //
		bool gateway_managed;                   // Is this deviced managed by the set of gateway accounts
		uint32 authorizedDevicesIndex;          //
	}


	/*
	 *
	 */
	struct AdminUser {
		bool isAdmin;                           //
		uint32 authorizedAdminUsersIndex;       // The index within active_admin_users which points to this AdminUser
	}

	uint32 defaultDataReceiver;

	uint32 next_device_id;
	uint32[] free_device_id_stack;
	mapping (address => uint32) addr_to_id;
	mapping (uint32 => Device) id_to_device;

	mapping (address => bool) gateway_pool;

	mapping (address => AdminUser) admin_mapping;
	address[] active_admin_users;

	uint32[] authorized_devices;
	uint32[] authorized_gateways;



	// TODO: Should we index any of these event topics?

	// Keccak256 Signature: 9ff928f5be58747f4db3b34d53225d1e1cf1562a3158bd45c2934fca5907f919
	event Add_Device				(address indexed msgSender, address clientAddr, string name, bool gateway_managed, bool gateway, uint32 device_id);

	// Keccak256 Signature: c3d811754f31d6181381ab5fbf732898911891abe7d32e97de73a1ea84c2e363
	event Remove_Device				(address indexed msgSender, uint32 device_id);

	// Keccak256 Signature: 0d014d0489a2ad2061dbf1dffe20d304792998e0635b29eda36a724992b6e5c9
	event Remove_Gateway			(address indexed msgSender, address gateway_addr, uint32 device_id);

	// Keccak256 Signature: bba4d289b156cad6df20a164dc91021ab64d1c7d594ddd9128fca71d6366b3c9
	event Push_Data					(address indexed msgSender, uint32 device_id);

	// Keccak256 Signature: 98e87e8e01e32169ab3efa3535263beca76318be8ede6a321bcbfcbf1e499d26
	event Device_Data_Updated		(uint32 indexed dataReceiverID, uint32 device_id);

	// Keccak256 Signature: e21f6cd2771fa3b4f5641e2fd1a3d52156a9a8cc10da311d5de41a5755ca6acf
	event Update_DataReceiver		(address indexed msgSender, uint32 device_id, uint32 dataReceiver);

	// Keccak256 Signature: adf201dc3ee5a3915c67bf861b4c0ec432dded7b6a82938956e1f411c5636287
	event Set_Default_DataReceiver	(address indexed msgSender, uint32 device_id);

	// Keccak256 Signature: f873df4dfb480f3a05c2bde3cae4779f61d6b60c3f6a0f1ab57aac0fe021a686
	event Update_Addr				(address indexed msgSender, uint32 device_id, uint32 addrType, string addr);

	// Keccak256 Signature: 9f99e7c31d775c4f75816a8e1a0655e1e5f5bab88311d820d261ebab2ae8d91f
	event Update_PublicKey			(address indexed msgSender, uint32 device_id, string newPublicKey);

	// Keccak256 Signature: 7abf29af9412d237982695e733aed1af1c9ba2c4df30a4a18b76f478c7ace921
	event Update_SignKey			(address indexed msgSender, uint32 device_id, string newSignPublicKey);

	// Keccak256 Signature: 134c4a950d896d7c32faa850baf4e3bccf293ae2538943709726e9596ce9ebaf
	event Authorize_Admin			(address indexed msgSender, address newAdminAddr);

	// Keccak256 Signature: e96008d87980c624fca6a2c0ecc59bcef2ef54659e80a1333aff845ea113f160
	event Deauthorize_Admin			(address indexed msgSender, address adminAddr);




	/*
	 * @dev
	 */
	constructor() public {
		defaultDataReceiver = 0;
		next_device_id = 1; // A device_id of 0 indicates unassigned (ex: if addr_to_id[addr] == 0 : unassigned)
		admin_mapping[msg.sender].isAdmin = true;
		active_admin_users.push(msg.sender);
		admin_mapping[msg.sender].authorizedAdminUsersIndex = uint32(active_admin_users.length - 1);
	}


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
		require(id_to_device[addr_to_id[msg.sender]].active || gateway_pool[msg.sender] || admin_mapping[msg.sender].isAdmin);
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
			require(id_to_device[device_id].gateway_managed || id_to_device[device_id].eth_addr == msg.sender || admin_mapping[msg.sender].isAdmin);
		} else {
			require(id_to_device[device_id].eth_addr == msg.sender || admin_mapping[msg.sender].isAdmin);
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


	//TODO: Which of fallback and/or receive is needed?
	//fallback() external payable {}

	//TODO: ETHABI fails to encode calls when this exists as it currently stands in ABI form
	//receive() external payable {}


	 /*
	  * @dev Is the given address an admin? Only for external calls. For internal use, test: admin_mapping[clientAddr].isAdmin
	  * @param clientAddr
	  * @return
	  */
	function is_admin(address clientAddr) external view returns(bool) {
		return admin_mapping[clientAddr].isAdmin;
	}


	/*
	 * @dev Is the given address an authorized device? Only for external calls. For internal use, test: id_to_device[clientAddr].active
	 * @param
	 * @return
	 */
	function is_authd(uint32 device_id) external view returns(bool) {
		return id_to_device[device_id].active;
	}


	/*
	 * @dev
	 * @param
	 * @return
	 */
	function is_device(uint32 device_id) external view returns(bool) {
		return id_to_device[device_id].active && !id_to_device[device_id].is_gateway;
	}


	/*
	 * @dev
	 * @param
	 * @return
	 */
	function is_gateway(uint32 device_id) external view returns(bool) {
		return id_to_device[device_id].active && id_to_device[device_id].is_gateway;
	}


	/*
	 * @dev
	 * @param
	 * @return
	 */
	function is_gateway_managed(uint32 device_id) external view _authorized returns(bool) {
		return id_to_device[device_id].gateway_managed;
	}


	/*
	 * @dev
	 * @param
	 * @return
	 */
	function get_default_datareceiver() external view _authorized returns(uint32) {
		return defaultDataReceiver;
	}


	/*
	 * @dev
	 * @param
	 * @return
	 */
	function get_my_device_id() external view returns(uint32) {
		require(id_to_device[addr_to_id[msg.sender]].active);
		return addr_to_id[msg.sender];
	}


	/*
	 * @dev
	 * @param
	 * @return
	 */
	function get_name(uint32 device_id) external view _authorized returns(string memory) {
		require(id_to_device[device_id].active);
		return id_to_device[device_id].name;
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
	function get_signkey(uint32 device_id) external view _authorized returns(string memory) {
		require(id_to_device[device_id].active);
		return id_to_device[device_id].signPublicKey;
	}


	/*
	 * @dev
	 * @param
	 * @return
	 */
	function get_datareceiver(uint32 device_id) external view _authorized returns(uint32) { // TODO: Modifiers might need adjusting here
		require(id_to_device[device_id].active);
		return id_to_device[device_id].dataReceiverID;
	}


	/*
	 * @dev
	 * @param
	 * @return
	 */
	function get_addrtype(uint32 device_id) external view _authorized returns(uint8) {
		require(id_to_device[device_id].active);
		return uint8(id_to_device[device_id].addrType);
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


	// TODO: Modifier should probably be _authorizedDeviceOrGateway
	/*
	 * @dev
	 * @param
	 * @return
	 */
	function get_data(uint32 device_id) external view _authorized returns(string memory, uint16, string memory) {
		require(id_to_device[device_id].active);
		return (id_to_device[device_id].data, id_to_device[device_id].dataLen, id_to_device[device_id].nonce);
	}


	// TODO: Modifier should probably be _authorizedDeviceOrGateway
	/*
	 * @dev
	 * @param
	 * @return
	 */
	function get_dataTimestamp(uint32 device_id) external view _authorized returns(uint256) {
		require(id_to_device[device_id].active);
		return id_to_device[device_id].dataTimestamp;
	}


	/*
	 * @dev
	 * @param
	 * @return
	 */
	function get_creationTimestamp(uint32 device_id) external view _authorized returns(uint256) {
		require(id_to_device[device_id].active);
		return id_to_device[device_id].creationTimestamp;
	}


	/*
	 * @dev
	 * @param clientAddr
	 * @param name
	 * @param publicKey
	 * @param gateway_managed
	 * @return
	 */
	function add_device(address clientAddr, string calldata name, bool gateway_managed, bool gateway) external _admin returns(uint32) {
		require(gateway_managed || addr_to_id[clientAddr] == 0 || !id_to_device[addr_to_id[clientAddr]].active);
		uint32 device_id;

		if (free_device_id_stack.length > 0) {
			device_id = free_device_id_stack[free_device_id_stack.length - 1];
			free_device_id_stack.pop();
		} else {
			device_id = next_device_id++;
		}

		id_to_device[device_id].device_id = device_id;
		id_to_device[device_id].name = name;
		id_to_device[device_id].active = true;
		id_to_device[device_id].is_gateway = gateway && !gateway_managed;
		id_to_device[device_id].dataReceiverID = defaultDataReceiver;
		id_to_device[device_id].creationTimestamp = uint32(now);
		id_to_device[device_id].gateway_managed = gateway_managed;

		if (gateway && !gateway_managed) {
			// Gateway
			authorized_gateways.push(device_id);
			id_to_device[device_id].authorizedDevicesIndex = uint32(authorized_gateways.length - 1);

			gateway_pool[clientAddr] = true;
			addr_to_id[clientAddr] = device_id;

		} else {
			// Device
			authorized_devices.push(device_id);
			addr_to_id[clientAddr] = device_id;
			id_to_device[device_id].authorizedDevicesIndex = uint32(authorized_devices.length - 1);
		}

		if (gateway_managed) {
			id_to_device[device_id].has_eth_addr = false;
		} else {
			id_to_device[device_id].eth_addr = clientAddr;
			addr_to_id[clientAddr] = device_id;
			id_to_device[device_id].has_eth_addr = true;
		}

		emit Add_Device(msg.sender, clientAddr, name, gateway_managed, gateway, device_id);
		return device_id;
	}


	/*
	 * @dev
	 * @param clientAddr
	 * @return
	 */
	function remove_device(uint32 device_id) external _admin returns(bool) {
		require(id_to_device[device_id].active);

		// TODO: Leave or remove these memory variables?
		uint32 authorizedDevicesIndex = uint32(id_to_device[device_id].authorizedDevicesIndex);
		uint32 lastAuthorizedDevicesIndex = uint32(authorized_devices.length - 1);
		uint32 lastAuthorizedDevice = uint32(authorized_devices[lastAuthorizedDevicesIndex]);
		authorized_devices[authorizedDevicesIndex] = authorized_devices[lastAuthorizedDevicesIndex];
		id_to_device[lastAuthorizedDevice].authorizedDevicesIndex = authorizedDevicesIndex;
		if (defaultDataReceiver == device_id) {
			defaultDataReceiver = 0;
		}
		authorized_devices.pop();

		free_device_id_stack.push(device_id);
		if (id_to_device[device_id].has_eth_addr) {
			delete addr_to_id[id_to_device[device_id].eth_addr];
		}
		delete id_to_device[device_id];

		emit Remove_Device(msg.sender, device_id);
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

		// TODO: Leave or remove these memory variables?
		uint32 authorizedDevicesIndex = uint32(id_to_device[device_id].authorizedDevicesIndex);
		uint32 lastAuthorizedDevicesIndex = uint32(authorized_gateways.length - 1);
		uint32 lastAuthorizedDevice = uint32(authorized_gateways[lastAuthorizedDevicesIndex]);
		authorized_gateways[authorizedDevicesIndex] = authorized_gateways[lastAuthorizedDevicesIndex];
		id_to_device[lastAuthorizedDevice].authorizedDevicesIndex = authorizedDevicesIndex;
		if (defaultDataReceiver == device_id) {
			defaultDataReceiver = 0;
		}
		authorized_gateways.pop();

		free_device_id_stack.push(device_id);
		delete gateway_pool[gateway_addr];
		delete id_to_device[device_id];
		delete addr_to_id[gateway_addr];

		emit Remove_Gateway(msg.sender, gateway_addr, device_id);
		return true;
	}


	/*
	 * @dev
	 * @param
	 * @return
	 */
	function update_datareceiver(uint32 device_id, uint32 dataReceiverID) external _admin returns(bool) {
		require(id_to_device[dataReceiverID].active);
		id_to_device[device_id].dataReceiverID = dataReceiverID;

		emit Update_DataReceiver(msg.sender, device_id, dataReceiverID);
		return true;
	}


	/*
	 * @dev
	 * @param
	 * @param addrType
	 * @param addr
	 * @return
	 */
	function update_addr(uint32 device_id, uint32 addrType, string calldata addr) external _authorizedDeviceOrGateway _mutator(device_id) returns(bool) {
		id_to_device[device_id].addrType = AddrType(addrType);
		id_to_device[device_id].addr = addr;

		emit Update_Addr(msg.sender, device_id, addrType, addr);
		return true;
	}


	/*
	 * @dev
	 * @param
	 * @param newPublicKey
	 * @return
	 */
	function update_publickey(uint32 device_id, string calldata newPublicKey) external _authorized _mutator(device_id) returns(bool) {
		id_to_device[device_id].publicKey = newPublicKey;

		emit Update_PublicKey(msg.sender, device_id, newPublicKey);
		return true;
	}


	/*
	 * @dev
	 * @param
	 * @param newSignPublicKey
	 * @return
	 */
	function update_signkey(uint32 device_id, string calldata newSignPublicKey) external _authorized _mutator(device_id) returns(bool) {
		id_to_device[device_id].signPublicKey = newSignPublicKey;

		emit Update_SignKey(msg.sender, device_id, newSignPublicKey);
		return true;
	}


	/*
	 * @dev
	 * @param addrType
	 * @param addr
	 * @return
	 */
	function push_data(uint32 device_id, string calldata data, uint16 dataLen, string calldata nonce) external _authorizedDeviceOrGateway _mutator(device_id) returns(bool) {
		id_to_device[device_id].data = data;
		id_to_device[device_id].dataLen = dataLen;
		id_to_device[device_id].nonce = nonce;
		id_to_device[device_id].dataTimestamp = uint32(now);

		//emit Push_Data(msg.sender, device_id, uint32(now), data); //TODO: Data will be stored as a log AND data? Is just a log sufficient?
		emit Push_Data(msg.sender, device_id);
		emit Device_Data_Updated(id_to_device[device_id].dataReceiverID, device_id);
		return true;
	}


	/*
	 * @dev
	 * @param
	 * @return
	 */
	function set_default_datareceiver(uint32 dataReceiverID) external _admin returns(bool) {
		require(id_to_device[dataReceiverID].active);
		defaultDataReceiver = dataReceiverID;

		emit Set_Default_DataReceiver(msg.sender, dataReceiverID);
		return true;
	}


	/*
	 * @dev
	 * @param newAdminAddr
	 * @return
	 */
	function authorize_admin(address newAdminAddr) external _admin returns(bool) {
		require(!admin_mapping[newAdminAddr].isAdmin);
		admin_mapping[newAdminAddr].isAdmin = true;
		active_admin_users.push(newAdminAddr);
		admin_mapping[newAdminAddr].authorizedAdminUsersIndex = uint32(active_admin_users.length - 1);

		emit Authorize_Admin(msg.sender, newAdminAddr);
		return true;
	}


	/*
	 * @dev
	 * @param adminAddr
	 * @return
	 */
	function deauthorize_admin(address adminAddr) external _admin returns(bool) {
		require(admin_mapping[adminAddr].isAdmin);
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

		emit Deauthorize_Admin(msg.sender, adminAddr);
		return true;
	}


	/*
	 * @dev
	 * @return
	 */
	function get_num_admin() external view _admin returns(uint32) {
		return uint32(active_admin_users.length);
	}


	/*
	 * @dev
	 * @return
	 */
	function get_num_devices() external view _admin returns(uint32) {
		return uint32(authorized_devices.length);
	}


	/*
	 * @dev
	 * @return
	 */
	function get_num_gateways() external view _admin returns(uint32) {
		return uint32(authorized_gateways.length);
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
	function get_authorized_devices() public view _admin returns(uint32[] memory) {
		return authorized_devices;
	}


	/*
	 * @dev
	 * @return
	 */
	function get_authorized_gateways() public view _admin returns(uint32[] memory) {
		return authorized_gateways;
	}

}
