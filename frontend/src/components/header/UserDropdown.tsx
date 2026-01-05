import { useState } from "react";
import { ListPlus, ListMinus, LogOut, UserPen, PiggyBank } from "lucide-react";
import { DropdownItem } from "../ui/dropdown/DropdownItem";
import { Dropdown } from "../ui/dropdown/Dropdown";
import { useAuth } from "../../auth/AuthContext";

export default function UserDropdown() {
  const [isOpen, setIsOpen] = useState(false);

  const { user, logout } = useAuth();

  function toggleDropdown() {
    setIsOpen(!isOpen);
  }

  function closeDropdown() {
    setIsOpen(false);
  }
  return (
    <div className="relative">
      <button
        onClick={toggleDropdown}
        className="flex items-center text-gray-700 dropdown-toggle dark:text-gray-400"
      >
        {!isOpen ? (
          <ListPlus size={20} className="text-gray-500" />
        ) : (
          <ListMinus size={20} className="text-gray-500" />
        )}
      </button>

      <Dropdown
        isOpen={isOpen}
        onClose={closeDropdown}
        className="absolute right-0 mt-[17px] flex w-[260px] flex-col rounded-2xl border border-gray-200 bg-white p-3 shadow-theme-lg dark:border-gray-800 dark:bg-gray-dark"
      >
        <span className="block mr-1 font-medium text-theme-sm">
          {user?.email}
        </span>
        <ul className="flex flex-col gap-1 pt-4 pb-3 border-b border-gray-200 dark:border-gray-800">
          <li>
            <DropdownItem
              onItemClick={closeDropdown}
              tag="a"
              to="/profile"
              className="flex items-center gap-3 px-3 py-2 font-medium text-gray-700 rounded-lg group text-theme-sm hover:bg-gray-100 hover:text-gray-700 dark:text-gray-400 dark:hover:bg-white/5 dark:hover:text-gray-300"
            >
              <UserPen size={20} className="text-gray-500" />
              Edit profile
            </DropdownItem>
            <DropdownItem
              onItemClick={closeDropdown}
              tag="a"
              to="/subscription"
              className="flex items-center gap-3 px-3 py-2 font-medium text-gray-700 rounded-lg group text-theme-sm hover:bg-gray-100 hover:text-gray-700 dark:text-gray-400 dark:hover:bg-white/5 dark:hover:text-gray-300"
            >
              <PiggyBank size={20} className="text-gray-500" />
              Subscription
            </DropdownItem>
          </li>
        </ul>
        <a
          onClick={logout}
          className="flex items-center gap-3 px-3 py-2 mt-3 font-medium text-gray-700 rounded-lg group text-theme-sm hover:bg-gray-100 hover:text-gray-700 dark:text-gray-400 dark:hover:bg-white/5 dark:hover:text-gray-300"
        >
          <LogOut size={20} className="text-gray-500" />
          Sign out
        </a>
      </Dropdown>
    </div>
  );
}
