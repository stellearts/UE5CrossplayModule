// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

struct SampleConstants
{
	/** The product id for the running application, found on the dev portal */
	static constexpr char ProductId[] = "cf6e819c86fc45548c72029b93e59853";

	/** The sandbox id for the running application, found on the dev portal */
	static constexpr char SandboxId[] = "3bd37bf86718447bbb4f5d7473212ec5";

	/** The deployment id for the running application, found on the dev portal */
	static constexpr char DeploymentId[] = "7c5d0e6b46f2429aab485c838008d458";

	/** Client id of the service permissions entry, found on the dev portal */
	static constexpr char ClientCredentialsId[] = "xyza7891cHQIAxVpn4iwHSPLv4JGOZp6";

	/** Client secret for accessing the set of permissions, found on the dev portal */
	static constexpr char ClientCredentialsSecret[] = "RnQd8t2c/OR8qKpQ449re2zln9lheKHL5G9i9x/A5H4";

	/** Game name */
	static constexpr char GameName[] = "MBGame";

	/** Encryption key. Not used by this sample. */
	static constexpr char EncryptionKey[] = "1111111111111111111111111111111111111111111111111111111111111111";
};