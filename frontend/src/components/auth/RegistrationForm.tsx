import { useState } from "react";
import { useAuth } from "../../auth/AuthContext";
import { Eye, EyeOff } from "lucide-react";
import Label from "../form/Label";
import Input from "../form/input/InputField";
import Button from "../ui/button/Button";

export default function RegistrationForm() {
  const [showPassword, setShowPassword] = useState(false);
  const [firstName, setFirstName] = useState("");
  const [familyName, setFamilyName] = useState("");
  const [email, setEmail] = useState("");
  const [password, setPassword] = useState("");
  const [error, setError] = useState("");
  const { register } = useAuth();

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    console.log(firstName);
    console.log(familyName);
    console.log(email);
    console.log(password);
    const success = await register(firstName, familyName, email, password);
    if (!success) {
      setError("Invalid credentials. Please try again.");
    }
  };

  return (
    <div className="flex flex-col flex-1">
      <div className="w-full max-w-md pt-10 mx-auto"></div>
      <div className="flex flex-col justify-center flex-1 w-full max-w-md mx-auto">
        <div>
          <div className="mb-5 sm:mb-8">
            <h1 className="mb-2 font-semibold text-gray-800 text-title-sm dark:text-white/90 sm:text-title-md">
              Welcome!
            </h1>
            <p>Enter your details below to create an account with us.</p>
          </div>
          {error && (
            <div className="mb-4 text-sm text-red-600 dark:text-red-400">
              {error}
            </div>
          )}
          <form onSubmit={handleSubmit} className="mb-4">
            <div className="space-y-6">
              <div>
                <Label>
                  First name <span className="text-error-500">*</span>
                </Label>
                <Input
                  placeholder="First name"
                  value={firstName}
                  onChange={(e) => setFirstName(e.target.value)}
                />
              </div>
              <div>
                <Label>
                  Family name <span className="text-error-500">*</span>
                </Label>
                <Input
                  placeholder="Family name"
                  value={familyName}
                  onChange={(e) => setFamilyName(e.target.value)}
                />
              </div>
              <div>
                <Label>
                  Email <span className="text-error-500">*</span>
                </Label>
                <Input
                  placeholder="email@example.com"
                  value={email}
                  onChange={(e) => setEmail(e.target.value)}
                />
              </div>
              <div>
                <Label>
                  Password <span className="text-error-500">*</span>
                </Label>
                <div className="relative">
                  <Input
                    type={showPassword ? "text" : "password"}
                    value={password}
                    onChange={(e) => setPassword(e.target.value)}
                  />
                  <span
                    onClick={() => setShowPassword(!showPassword)}
                    className="absolute z-30 -translate-y-1/2 cursor-pointer right-4 top-1/2"
                  >
                    {showPassword ? (
                      <Eye size={16} className="mr-1" />
                    ) : (
                      <EyeOff size={16} className="mr-1" />
                    )}
                  </span>
                </div>
              </div>
              <div>
                <Button className="w-full" size="sm">
                  Register
                </Button>
              </div>
            </div>
          </form>
        </div>
      </div>
    </div>
  );
}
